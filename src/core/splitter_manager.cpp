#include "splitter_manager.h"
#include "../utils/logger.h"

#include <algorithm>
#include <chrono>

namespace splitter::core {

SplitterManager::SplitterManager(ConfigManager* config)
    : config_(config)
    , last_frequency_update_(std::chrono::steady_clock::now())
    , last_health_check_(std::chrono::steady_clock::now()) {
}

SplitterManager::~SplitterManager() {
    if (initialized_) {
        shutdown();
    }
}

bool SplitterManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        gpio_controller_ = std::make_unique<hardware::GpioController>();
        if (!gpio_controller_->initialize()) {
            utils::Logger::error("Failed to initialize GPIO controller");
            return false;
        }
        
        frequency_detector_ = std::make_unique<hardware::FrequencyDetector>();
        if (!frequency_detector_->initialize()) {
            utils::Logger::error("Failed to initialize frequency detector");
            return false;
        }
        
        led_controller_ = std::make_unique<hardware::LedController>(gpio_controller_.get());
        if (!led_controller_->initialize()) {
            utils::Logger::error("Failed to initialize LED controller");
            return false;
        }
        
        ports_.reserve(NUM_PORTS);
        for (int i = 0; i < NUM_PORTS; ++i) {
            auto port = std::make_unique<Port>(i, gpio_controller_.get(), led_controller_.get());
            if (!port->initialize()) {
                utils::Logger::error("Failed to initialize port {}", i);
                return false;
            }
            ports_.push_back(std::move(port));
        }
        
        initialized_ = true;
        utils::Logger::info("SplitterManager initialized successfully with {} ports", NUM_PORTS);
        return true;
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception during initialization: {}", e.what());
        return false;
    }
}

void SplitterManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    shutdown_requested_ = true;
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    for (auto& port : ports_) {
        if (port) {
            port->disable();
        }
    }
    
    ports_.clear();
    led_controller_.reset();
    frequency_detector_.reset();
    gpio_controller_.reset();
    
    initialized_ = false;
    utils::Logger::info("SplitterManager shutdown complete");
}

bool SplitterManager::enable_port(int port_id) {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        utils::Logger::warning("Invalid port ID: {}", port_id);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    if (!ports_[port_id]) {
        utils::Logger::error("Port {} not initialized", port_id);
        return false;
    }
    
    if (ports_[port_id]->enable()) {
        auto state = ports_[port_id]->get_state();
        notify_state_change(port_id, state);
        utils::Logger::info("Port {} enabled", port_id);
        return true;
    }
    
    utils::Logger::error("Failed to enable port {}", port_id);
    return false;
}

bool SplitterManager::disable_port(int port_id) {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        utils::Logger::warning("Invalid port ID: {}", port_id);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    if (!ports_[port_id]) {
        utils::Logger::error("Port {} not initialized", port_id);
        return false;
    }
    
    if (ports_[port_id]->disable()) {
        auto state = ports_[port_id]->get_state();
        notify_state_change(port_id, state);
        utils::Logger::info("Port {} disabled", port_id);
        return true;
    }
    
    utils::Logger::error("Failed to disable port {}", port_id);
    return false;
}

bool SplitterManager::is_port_enabled(int port_id) const {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    return ports_[port_id] && ports_[port_id]->is_enabled();
}

PortState SplitterManager::get_port_state(int port_id) const {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        return {};
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    return ports_[port_id] ? ports_[port_id]->get_state() : PortState{};
}

std::vector<PortState> SplitterManager::get_all_port_states() const {
    std::vector<PortState> states;
    states.reserve(NUM_PORTS);
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    for (const auto& port : ports_) {
        states.push_back(port ? port->get_state() : PortState{});
    }
    
    return states;
}

double SplitterManager::get_port_frequency(int port_id) const {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        return 0.0;
    }
    
    return frequency_detector_ ? frequency_detector_->get_frequency(port_id) : 0.0;
}

bool SplitterManager::set_port_configuration(int port_id, const PortConfig& config) {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    if (!ports_[port_id]) {
        return false;
    }
    
    return ports_[port_id]->set_configuration(config);
}

PortConfig SplitterManager::get_port_configuration(int port_id) const {
    if (port_id < 0 || port_id >= NUM_PORTS) {
        return {};
    }
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    return ports_[port_id] ? ports_[port_id]->get_configuration() : PortConfig{};
}

void SplitterManager::register_state_change_callback(StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    state_callbacks_.push_back(std::move(callback));
}

void SplitterManager::register_frequency_change_callback(FrequencyChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    frequency_callbacks_.push_back(std::move(callback));
}

void SplitterManager::process_events() {
    if (!initialized_ || shutdown_requested_) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    if (now - last_frequency_update_ >= std::chrono::milliseconds(500)) {
        update_port_frequencies();
        last_frequency_update_ = now;
    }
    
    if (now - last_health_check_ >= std::chrono::seconds(5)) {
        check_port_health();
        last_health_check_ = now;
    }
}

bool SplitterManager::get_system_health() const {
    if (!initialized_) {
        return false;
    }
    
    return gpio_controller_->is_healthy() && 
           frequency_detector_->is_healthy() && 
           led_controller_->is_healthy();
}

SystemStats SplitterManager::get_system_stats() const {
    SystemStats stats;
    stats.hardware_healthy = get_system_health();
    
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    uint32_t active_count = 0;
    double total_frequency = 0.0;
    
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (ports_[i] && ports_[i]->is_enabled()) {
            ++active_count;
            total_frequency += get_port_frequency(i);
        }
    }
    
    stats.active_ports = active_count;
    stats.average_frequency_mhz = active_count > 0 ? total_frequency / active_count : 0.0;
    
    return stats;
}

void SplitterManager::update_port_frequencies() {
    if (!frequency_detector_) {
        return;
    }
    
    for (int i = 0; i < NUM_PORTS; ++i) {
        if (is_port_enabled(i)) {
            double frequency = frequency_detector_->measure_frequency(i);
            notify_frequency_change(i, frequency);
        }
    }
}

void SplitterManager::check_port_health() {
    std::lock_guard<std::mutex> lock(ports_mutex_);
    
    for (auto& port : ports_) {
        if (port) {
            port->check_health();
        }
    }
}

void SplitterManager::notify_state_change(int port_id, const PortState& state) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    
    for (const auto& callback : state_callbacks_) {
        try {
            callback(port_id, state);
        } catch (const std::exception& e) {
            utils::Logger::error("Exception in state change callback: {}", e.what());
        }
    }
}

void SplitterManager::notify_frequency_change(int port_id, double frequency) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    
    for (const auto& callback : frequency_callbacks_) {
        try {
            callback(port_id, frequency);
        } catch (const std::exception& e) {
            utils::Logger::error("Exception in frequency change callback: {}", e.what());
        }
    }
}

} // namespace splitter::core