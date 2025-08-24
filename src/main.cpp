#include "core/splitter_system.h"
#include "netconf/netconf_server.h"
#include "web/rest_server.h"
#include <iostream>
#include <memory>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <fstream>

class SplitterDaemon {
public:
    SplitterDaemon() : running_(true) {
        splitter_system_ = std::make_shared<core::SplitterSystem>();
        netconf_server_ = std::make_unique<netconf::NetConfServer>(splitter_system_);
        web_server_ = std::make_unique<web::RestServer>(splitter_system_);
    }
    
    bool initialize(const std::string& config_file, bool debug_mode) {
        debug_mode_ = debug_mode;
        
        if (debug_mode_) {
            std::cout << "Starting L-Band Splitter in debug mode" << std::endl;
        }
        
        // Load configuration
        if (!load_configuration(config_file)) {
            std::cerr << "Failed to load configuration from: " << config_file << std::endl;
            return false;
        }
        
        // Initialize splitter system
        if (!splitter_system_->initialize()) {
            std::cerr << "Failed to initialize splitter system" << std::endl;
            return false;
        }
        
        // Initialize NetConf server
        if (!netconf_server_->initialize("0.0.0.0", 830)) {
            std::cerr << "Failed to initialize NetConf server" << std::endl;
            return false;
        }
        
        // Initialize web server
        if (!web_server_->initialize("0.0.0.0", 8080)) {
            std::cerr << "Failed to initialize web server" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool start() {
        std::cout << "Starting L-Band Splitter services..." << std::endl;
        
        // Start NetConf server
        if (!netconf_server_->start()) {
            std::cerr << "Failed to start NetConf server" << std::endl;
            return false;
        }
        
        // Start web server
        if (!web_server_->start()) {
            std::cerr << "Failed to start web server" << std::endl;
            return false;
        }
        
        std::cout << "L-Band Splitter services started successfully" << std::endl;
        std::cout << "NetConf server: port 830" << std::endl;
        std::cout << "Web interface: http://localhost:8080" << std::endl;
        
        return true;
    }
    
    void run() {
        std::cout << "L-Band Splitter is running. Press Ctrl+C to stop." << std::endl;
        
        while (running_) {
            sleep(1);
        }
    }
    
    void stop() {
        std::cout << "Stopping L-Band Splitter services..." << std::endl;
        
        running_ = false;
        
        if (web_server_) {
            web_server_->stop();
        }
        
        if (netconf_server_) {
            netconf_server_->stop();
        }
        
        if (splitter_system_) {
            splitter_system_->shutdown();
        }
        
        std::cout << "L-Band Splitter stopped" << std::endl;
    }

private:
    std::shared_ptr<core::SplitterSystem> splitter_system_;
    std::unique_ptr<netconf::NetConfServer> netconf_server_;
    std::unique_ptr<web::RestServer> web_server_;
    
    bool running_;
    bool debug_mode_;
    
    bool load_configuration(const std::string& config_file) {
        // Basic configuration loading
        // In a real implementation, this would parse a configuration file
        // and set up system parameters
        
        std::ifstream file(config_file);
        if (!file.is_open()) {
            if (debug_mode_) {
                std::cout << "Configuration file not found, using defaults" << std::endl;
            }
            return true; // Use defaults if config file doesn't exist
        }
        
        if (debug_mode_) {
            std::cout << "Loaded configuration from: " << config_file << std::endl;
        }
        
        return true;
    }
};

// Global daemon instance for signal handling
SplitterDaemon* g_daemon = nullptr;

void signal_handler(int signal) {
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
            if (g_daemon) {
                g_daemon->stop();
            }
            break;
        default:
            break;
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\n"
              << "L-Band Splitter/Combiner Control Daemon\n"
              << "\n"
              << "Options:\n"
              << "  -c, --config FILE    Configuration file path (default: /etc/splitter/splitter.conf)\n"
              << "  -d, --debug          Enable debug mode\n"
              << "  -h, --help           Show this help message\n"
              << "  -v, --version        Show version information\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << "                           # Run with default configuration\n"
              << "  " << program_name << " --config custom.conf      # Run with custom configuration\n"
              << "  " << program_name << " --debug                   # Run in debug mode\n"
              << std::endl;
}

void print_version() {
    std::cout << "L-Band Splitter Control Daemon v1.0.0\n"
              << "32-port L-Band Splitter/Combiner System\n"
              << "Built with C++17, NetConf, and Angular\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string config_file = "/etc/splitter/splitter.conf";
    bool debug_mode = false;
    
    // Command line argument parsing
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "c:dhv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                config_file = optarg;
                break;
            case 'd':
                debug_mode = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case '?':
                std::cerr << "Use --help for usage information." << std::endl;
                return 1;
            default:
                break;
        }
    }
    
    // Create daemon instance
    SplitterDaemon daemon;
    g_daemon = &daemon;
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Initialize the daemon
        if (!daemon.initialize(config_file, debug_mode)) {
            std::cerr << "Failed to initialize daemon" << std::endl;
            return 1;
        }
        
        // Start services
        if (!daemon.start()) {
            std::cerr << "Failed to start daemon services" << std::endl;
            return 1;
        }
        
        // Run main loop
        daemon.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
    
    return 0;
}