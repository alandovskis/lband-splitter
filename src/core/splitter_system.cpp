#include "splitter_system.h"
#include <iostream>
#include <algorithm>

namespace core {

SplitterSystem::SplitterSystem()
    : initialized_(false), monitoring_(false) {
    
    gpio_controller_ = std::make_shared<hardware::GPIOController>();
    freq_detector_array_ = std::make_shared<hardware::FrequencyDetectorArray>(NUM_PORTS);
}

SplitterSystem::~SplitterSystem() {
    shutdown();
}

bool SplitterSystem::initialize() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (initialized_.load()) {
        return true;
    }
    
    std::cout << "Initializing splitter system..." << std::endl;
    
    // Initialize GPIO controller
    if (!gpio_controller_->initialize()) {
        std::cerr << "Failed to initialize GPIO controller" << std::endl;
        return false;
    }
    
    // Initialize frequency detector array
    if (!freq_detector_array_->initialize()) {
        std::cerr << "Failed to initialize frequency detector array" << std::endl;
        return false;
    }
    
    // Initialize all ports
    ports_.clear();
    ports_.reserve(NUM_PORTS);
    
    for (int i = 0; i < NUM_PORTS; ++i) {
        int enable_led_pin = GPIO_BASE_ENABLE_LED + i;
        int signal_led_pin = GPIO_BASE_SIGNAL_LED + i;
        
        auto freq_detector = std::make_unique<hardware::FrequencyDetector>(i);
        
        auto port = std::make_unique<SplitterPort>(
            i, enable_led_pin, signal_led_pin,
            gpio_controller_, 
            std::move(freq_detector)
        );
        
        if (!port->initialize()) {
            std::cerr << "Failed to initialize port " << i << std::endl;
            return false;
        }
        
        ports_.push_back(std::move(port));
    }
    
    // Start monitoring thread
    monitoring_.store(true);
    monitor_thread_ = std::make_unique<std::thread>(&SplitterSystem::monitoring_loop, this);
    
    initialized_.store(true);
    std::cout << "Splitter system initialized with " << NUM_PORTS << " ports" << std::endl;
    
    return true;
}

void SplitterSystem::shutdown() {
    std::cout << "Shutting down splitter system..." << std::endl;
    
    // Stop monitoring
    monitoring_.store(false);
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
    
    // Disable all ports
    {
        std::lock_guard<std::mutex> lock(system_mutex_);
        for (auto& port : ports_) {
            port->disable();
        }
        ports_.clear();
    }
    
    initialized_.store(false);
    std::cout << "Splitter system shutdown complete" << std::endl;
}

bool SplitterSystem::enable_port(int port_number) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!validate_port_number(port_number)) {
        return false;
    }
    
    bool result = ports_[port_number]->enable();
    if (result) {
        notify_status_change();
    }
    
    return result;
}

bool SplitterSystem::disable_port(int port_number) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!validate_port_number(port_number)) {
        return false;
    }
    
    bool result = ports_[port_number]->disable();
    if (result) {
        notify_status_change();
    }
    
    return result;
}

bool SplitterSystem::enable_all_ports() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    bool success = true;
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (!ports_[i]->enable()) {
            success = false;
        }
    }
    
    if (success) {
        notify_status_change();
    }
    
    return success;
}

bool SplitterSystem::disable_all_ports() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    bool success = true;
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (!ports_[i]->disable()) {
            success = false;
        }
    }
    
    if (success) {
        notify_status_change();
    }
    
    return success;
}

SystemStatus SplitterSystem::get_system_status() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    SystemStatus status;
    status.port_statuses = get_all_port_statuses();
    status.enabled_ports = get_enabled_port_count();
    status.active_ports = get_active_port_count();
    status.system_healthy = initialized_.load();
    status.last_update = std::chrono::system_clock::now();
    
    return status;
}

PortStatus SplitterSystem::get_port_status(int port_number) const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!validate_port_number(port_number)) {
        PortStatus invalid_status;
        invalid_status.port_number = port_number;
        invalid_status.state = PortState::ERROR;
        invalid_status.error_message = "Invalid port number";
        return invalid_status;
    }
    
    return ports_[port_number]->get_status();
}

std::vector<PortStatus> SplitterSystem::get_all_port_statuses() const {
    std::vector<PortStatus> statuses;
    statuses.reserve(NUM_PORTS);
    
    for (int i = 0; i < NUM_PORTS; ++i) {
        statuses.push_back(ports_[i]->get_status());
    }
    
    return statuses;
}

bool SplitterSystem::is_port_enabled(int port_number) const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!validate_port_number(port_number)) {
        return false;
    }
    
    return ports_[port_number]->is_enabled();
}

bool SplitterSystem::is_signal_detected(int port_number) const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!validate_port_number(port_number)) {
        return false;
    }
    
    return ports_[port_number]->has_signal();
}

int SplitterSystem::get_enabled_port_count() const {
    int count = 0;
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (ports_[i]->is_enabled()) {
            count++;
        }
    }
    return count;
}

int SplitterSystem::get_active_port_count() const {
    int count = 0;
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (ports_[i]->has_signal()) {
            count++;
        }
    }
    return count;
}

bool SplitterSystem::set_port_configuration(int port_number, const std::map<std::string, std::string>& config) {
    // Implementation for port-specific configuration
    // This could include threshold settings, calibration values, etc.
    return validate_port_number(port_number);
}

std::map<std::string, std::string> SplitterSystem::get_port_configuration(int port_number) const {
    std::map<std::string, std::string> config;
    if (validate_port_number(port_number)) {
        // Return default configuration for now
        config["threshold_dbm"] = "-80.0";
        config["update_rate_ms"] = "100";
    }
    return config;
}

void SplitterSystem::register_status_callback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    status_callback_ = callback;
}

void SplitterSystem::unregister_status_callback() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    status_callback_ = nullptr;
}

bool SplitterSystem::run_self_test() {
    std::cout << "Running system self-test..." << std::endl;
    
    bool success = true;
    
    // Test GPIO controller
    if (!gpio_controller_) {
        success = false;
    }
    
    // Test frequency detectors
    if (!freq_detector_array_) {
        success = false;
    }
    
    // Test each port
    for (int i = 0; i < NUM_PORTS; ++i) {
        auto status = get_port_status(i);
        if (status.state == PortState::ERROR) {
            success = false;
        }
    }
    
    std::cout << "Self-test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success;
}

bool SplitterSystem::calibrate_frequency_detectors() {
    std::cout << "Calibrating frequency detectors..." << std::endl;
    
    // In a real implementation, this would perform calibration
    // of the ADCs and frequency measurement circuits
    
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate calibration time
    
    std::cout << "Frequency detector calibration complete" << std::endl;
    return true;
}

void SplitterSystem::monitoring_loop() {
    while (monitoring_.load()) {
        update_all_displays();
        notify_status_change();
        
        // Update every 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void SplitterSystem::update_all_displays() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    for (auto& port : ports_) {
        port->update_display();
    }
}

bool SplitterSystem::validate_port_number(int port_number) const {
    return port_number >= 0 && port_number < NUM_PORTS;
}

void SplitterSystem::notify_status_change() {
    if (status_callback_) {
        auto status = get_system_status();
        status_callback_(status);
    }
}

} // namespace core