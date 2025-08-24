#include "stm32_interface.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <chrono>

namespace hardware {

STM32Interface::STM32Interface(STM32Protocol protocol, const std::string& device_path)
    : protocol_(protocol)
    , device_path_(device_path)
    , device_fd_(-1)
    , connected_(false)
    , monitoring_(false)
    , timeout_ms_(1000)
    , max_retries_(3)
    , last_error_(STM32Status::SUCCESS)
    , comm_errors_(0) {
}

STM32Interface::~STM32Interface() {
    shutdown();
}

bool STM32Interface::initialize() {
    std::lock_guard<std::mutex> lock(comm_mutex_);
    
    if (connected_.load()) {
        return true;
    }
    
    std::cout << "Initializing STM32 interface via ";
    switch (protocol_) {
        case STM32Protocol::UART: std::cout << "UART"; break;
        case STM32Protocol::SPI: std::cout << "SPI"; break;
        case STM32Protocol::I2C: std::cout << "I2C"; break;
        case STM32Protocol::USB_CDC: std::cout << "USB CDC"; break;
    }
    std::cout << " on " << device_path_ << std::endl;
    
    if (!open_device()) {
        set_error(STM32Status::ERROR_HARDWARE_FAULT);
        return false;
    }
    
    // Test communication with system info command
    STM32Packet response;
    if (!send_command_with_response(STM32Command::GET_SYSTEM_INFO, {}, response)) {
        std::cerr << "Failed to communicate with STM32" << std::endl;
        close_device();
        return false;
    }
    
    connected_.store(true);
    monitoring_.store(true);
    
    // Start monitoring thread for status updates
    monitor_thread_ = std::make_unique<std::thread>(&STM32Interface::monitoring_thread, this);
    
    std::cout << "STM32 interface initialized successfully" << std::endl;
    return true;
}

void STM32Interface::shutdown() {
    std::cout << "Shutting down STM32 interface..." << std::endl;
    
    monitoring_.store(false);
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
    
    std::lock_guard<std::mutex> lock(comm_mutex_);
    connected_.store(false);
    close_device();
}

bool STM32Interface::enable_port(uint8_t port_number) {
    std::vector<uint8_t> data = {port_number};
    if (send_command(STM32Command::ENABLE_PORT, data)) {
        std::cout << "STM32: Enabled port " << static_cast<int>(port_number) << std::endl;
        return true;
    }
    return false;
}

bool STM32Interface::disable_port(uint8_t port_number) {
    std::vector<uint8_t> data = {port_number};
    if (send_command(STM32Command::DISABLE_PORT, data)) {
        std::cout << "STM32: Disabled port " << static_cast<int>(port_number) << std::endl;
        return true;
    }
    return false;
}

STM32PortStatus STM32Interface::get_port_status(uint8_t port_number) {
    STM32PortStatus status = {};
    status.port_number = port_number;
    
    std::vector<uint8_t> data = {port_number};
    STM32Packet response;
    
    if (send_command_with_response(STM32Command::GET_PORT_STATUS, data, response)) {
        if (response.data.size() >= 10) {
            status.enabled = response.data[0] & 0x01;
            status.signal_detected = response.data[0] & 0x02;
            status.enable_led_state = response.data[0] & 0x04;
            status.signal_led_state = response.data[0] & 0x08;
            status.error_flags = response.data[1];
            status.raw_adc_value = bytes_to_uint16(response.data, 2);
            status.frequency_mhz = bytes_to_float(response.data, 4);
            status.power_dbm = bytes_to_float(response.data, 8);
        }
    }
    
    return status;
}

std::vector<STM32PortStatus> STM32Interface::get_all_port_status() {
    std::vector<STM32PortStatus> statuses;
    
    STM32Packet response;
    if (send_command_with_response(STM32Command::GET_ALL_STATUS, {}, response)) {
        // Parse response for all 32 ports (assuming 12 bytes per port)
        const size_t bytes_per_port = 12;
        const size_t num_ports = response.data.size() / bytes_per_port;
        
        for (size_t i = 0; i < num_ports && i < 32; ++i) {
            STM32PortStatus status = {};
            size_t offset = i * bytes_per_port;
            
            status.port_number = static_cast<uint8_t>(i);
            status.enabled = response.data[offset] & 0x01;
            status.signal_detected = response.data[offset] & 0x02;
            status.enable_led_state = response.data[offset] & 0x04;
            status.signal_led_state = response.data[offset] & 0x08;
            status.error_flags = response.data[offset + 1];
            status.raw_adc_value = bytes_to_uint16(response.data, offset + 2);
            status.frequency_mhz = bytes_to_float(response.data, offset + 4);
            status.power_dbm = bytes_to_float(response.data, offset + 8);
            
            statuses.push_back(status);
        }
    }
    
    return statuses;
}

bool STM32Interface::set_enable_led(uint8_t port_number, bool state) {
    std::vector<uint8_t> data = {port_number, static_cast<uint8_t>(state ? 1 : 0)};
    return send_command(STM32Command::SET_ENABLE_LED, data);
}

bool STM32Interface::set_signal_led(uint8_t port_number, bool state) {
    std::vector<uint8_t> data = {port_number, static_cast<uint8_t>(state ? 1 : 0)};
    return send_command(STM32Command::SET_SIGNAL_LED, data);
}

float STM32Interface::get_frequency(uint8_t port_number) {
    std::vector<uint8_t> data = {port_number};
    STM32Packet response;
    
    if (send_command_with_response(STM32Command::GET_FREQUENCY, data, response)) {
        if (response.data.size() >= 4) {
            return bytes_to_float(response.data, 0);
        }
    }
    
    return 0.0f;
}

float STM32Interface::get_power_level(uint8_t port_number) {
    std::vector<uint8_t> data = {port_number};
    STM32Packet response;
    
    if (send_command_with_response(STM32Command::GET_POWER_LEVEL, data, response)) {
        if (response.data.size() >= 4) {
            return bytes_to_float(response.data, 0);
        }
    }
    
    return -100.0f; // Return very low power if read fails
}

bool STM32Interface::update_display(uint8_t port_number, const std::string& text) {
    std::vector<uint8_t> data = {port_number};
    
    // Add text data (limit to 16 characters for typical displays)
    size_t text_len = std::min(text.length(), static_cast<size_t>(16));
    for (size_t i = 0; i < text_len; ++i) {
        data.push_back(static_cast<uint8_t>(text[i]));
    }
    
    return send_command(STM32Command::UPDATE_DISPLAY, data);
}

STM32SystemInfo STM32Interface::get_system_info() {
    STM32SystemInfo info = {};
    
    STM32Packet response;
    if (send_command_with_response(STM32Command::GET_SYSTEM_INFO, {}, response)) {
        if (response.data.size() >= 20) {
            // Parse system info from response
            info.hardware_version = "STM32-v" + std::to_string(response.data[0]) + 
                                   "." + std::to_string(response.data[1]);
            info.firmware_version = std::to_string(response.data[2]) + "." + 
                                   std::to_string(response.data[3]) + "." + 
                                   std::to_string(response.data[4]);
            info.serial_number = bytes_to_uint32(response.data, 5);
            info.num_ports = bytes_to_uint16(response.data, 9);
            info.adc_resolution = bytes_to_uint16(response.data, 11);
            info.reference_voltage = bytes_to_float(response.data, 13);
            info.uptime_seconds = bytes_to_uint32(response.data, 17);
        }
    }
    
    return info;
}

bool STM32Interface::run_self_test() {
    STM32Packet response;
    if (send_command_with_response(STM32Command::RUN_SELF_TEST, {}, response)) {
        return response.data.size() > 0 && response.data[0] == 0x01; // Success flag
    }
    return false;
}

float STM32Interface::get_system_temperature() {
    STM32Packet response;
    if (send_command_with_response(STM32Command::GET_TEMPERATURE, {}, response)) {
        if (response.data.size() >= 4) {
            return bytes_to_float(response.data, 0);
        }
    }
    return 0.0f;
}

std::string STM32Interface::get_error_string() const {
    switch (last_error_) {
        case STM32Status::SUCCESS: return "Success";
        case STM32Status::ERROR_INVALID_COMMAND: return "Invalid command";
        case STM32Status::ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case STM32Status::ERROR_HARDWARE_FAULT: return "Hardware fault";
        case STM32Status::ERROR_TIMEOUT: return "Communication timeout";
        case STM32Status::ERROR_BUSY: return "System busy";
        case STM32Status::ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}

// Private methods implementation

bool STM32Interface::open_device() {
    switch (protocol_) {
        case STM32Protocol::UART:
            device_fd_ = open(device_path_.c_str(), O_RDWR | O_NOCTTY);
            if (device_fd_ < 0) return false;
            
            // Configure UART
            struct termios tty;
            if (tcgetattr(device_fd_, &tty) != 0) {
                close(device_fd_);
                return false;
            }
            
            cfsetospeed(&tty, B115200);
            cfsetispeed(&tty, B115200);
            
            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
            tty.c_iflag &= ~IGNBRK;
            tty.c_lflag = 0;
            tty.c_oflag = 0;
            tty.c_cc[VMIN] = 1;
            tty.c_cc[VTIME] = 1;
            
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            tty.c_cflag |= (CLOCAL | CREAD);
            tty.c_cflag &= ~(PARENB | PARODD);
            tty.c_cflag &= ~CSTOPB;
            tty.c_cflag &= ~CRTSCTS;
            
            if (tcsetattr(device_fd_, TCSANOW, &tty) != 0) {
                close(device_fd_);
                return false;
            }
            break;
            
        case STM32Protocol::SPI:
            device_fd_ = open(device_path_.c_str(), O_RDWR);
            if (device_fd_ < 0) return false;
            
            // Configure SPI
            uint32_t mode = SPI_MODE_0;
            uint8_t bits = 8;
            uint32_t speed = 1000000; // 1 MHz
            
            if (ioctl(device_fd_, SPI_IOC_WR_MODE32, &mode) == -1 ||
                ioctl(device_fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 ||
                ioctl(device_fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
                close(device_fd_);
                return false;
            }
            break;
            
        case STM32Protocol::I2C:
            device_fd_ = open(device_path_.c_str(), O_RDWR);
            if (device_fd_ < 0) return false;
            
            // Set I2C slave address (0x42 for STM32)
            if (ioctl(device_fd_, I2C_SLAVE, 0x42) < 0) {
                close(device_fd_);
                return false;
            }
            break;
            
        case STM32Protocol::USB_CDC:
            device_fd_ = open(device_path_.c_str(), O_RDWR);
            break;
    }
    
    return device_fd_ >= 0;
}

void STM32Interface::close_device() {
    if (device_fd_ >= 0) {
        close(device_fd_);
        device_fd_ = -1;
    }
}

bool STM32Interface::send_packet(const STM32Packet& packet) {
    std::vector<uint8_t> data;
    data.push_back(packet.header);
    data.push_back(packet.command);
    data.push_back(packet.length);
    data.insert(data.end(), packet.data.begin(), packet.data.end());
    data.push_back(packet.checksum);
    
    switch (protocol_) {
        case STM32Protocol::UART:
            return uart_send(data);
        case STM32Protocol::SPI:
            return spi_send(data);
        case STM32Protocol::I2C:
            return i2c_send(data);
        case STM32Protocol::USB_CDC:
            return usb_cdc_send(data);
    }
    
    return false;
}

bool STM32Interface::uart_send(const std::vector<uint8_t>& data) {
    ssize_t written = write(device_fd_, data.data(), data.size());
    return written == static_cast<ssize_t>(data.size());
}

bool STM32Interface::uart_receive(std::vector<uint8_t>& data, size_t expected_length) {
    data.resize(expected_length);
    
    size_t total_read = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (total_read < expected_length) {
        ssize_t bytes_read = read(device_fd_, data.data() + total_read, expected_length - total_read);
        if (bytes_read > 0) {
            total_read += bytes_read;
        }
        
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout_ms_) {
            set_error(STM32Status::ERROR_TIMEOUT);
            return false;
        }
        
        usleep(1000); // Small delay
    }
    
    return total_read == expected_length;
}

STM32Packet STM32Interface::create_command_packet(STM32Command cmd, const std::vector<uint8_t>& data) {
    STM32Packet packet;
    packet.header = 0xAA;
    packet.command = static_cast<uint8_t>(cmd);
    packet.length = data.size();
    packet.data = data;
    packet.checksum = calculate_checksum(packet);
    
    return packet;
}

bool STM32Interface::send_command(STM32Command cmd, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(comm_mutex_);
    
    if (!connected_.load()) {
        set_error(STM32Status::ERROR_NOT_INITIALIZED);
        return false;
    }
    
    auto packet = create_command_packet(cmd, data);
    return send_packet(packet);
}

uint8_t STM32Interface::calculate_checksum(const STM32Packet& packet) {
    uint8_t checksum = packet.header ^ packet.command ^ packet.length;
    for (uint8_t byte : packet.data) {
        checksum ^= byte;
    }
    return checksum;
}

void STM32Interface::set_error(STM32Status error) {
    last_error_ = error;
    if (error != STM32Status::SUCCESS) {
        increment_comm_error();
    }
}

void STM32Interface::increment_comm_error() {
    comm_errors_.fetch_add(1);
}

std::vector<uint8_t> STM32Interface::float_to_bytes(float value) {
    std::vector<uint8_t> bytes(4);
    memcpy(bytes.data(), &value, 4);
    return bytes;
}

float STM32Interface::bytes_to_float(const std::vector<uint8_t>& bytes, size_t offset) {
    if (bytes.size() < offset + 4) return 0.0f;
    
    float value;
    memcpy(&value, bytes.data() + offset, 4);
    return value;
}

void STM32Interface::monitoring_thread() {
    while (monitoring_.load()) {
        // Periodic status updates could be implemented here
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// Factory function implementation
std::unique_ptr<STM32Interface> create_stm32_interface(STM32Protocol protocol, const std::string& device_path) {
    return std::make_unique<STM32Interface>(protocol, device_path);
}

// Placeholder implementations for other protocols
bool STM32Interface::spi_send(const std::vector<uint8_t>& data) {
    // SPI implementation would go here
    return uart_send(data); // Fallback for now
}

bool STM32Interface::i2c_send(const std::vector<uint8_t>& data) {
    // I2C implementation would go here
    return uart_send(data); // Fallback for now
}

bool STM32Interface::usb_cdc_send(const std::vector<uint8_t>& data) {
    // USB CDC implementation would go here
    return uart_send(data); // Fallback for now
}

bool STM32Interface::send_command_with_response(STM32Command cmd, const std::vector<uint8_t>& data, STM32Packet& response) {
    if (!send_command(cmd, data)) {
        return false;
    }
    
    // Simplified response handling - in real implementation would properly parse response packets
    std::vector<uint8_t> response_data;
    if (uart_receive(response_data, 64)) { // Assume max 64 byte response
        response.data = response_data;
        return true;
    }
    
    return false;
}

} // namespace hardware