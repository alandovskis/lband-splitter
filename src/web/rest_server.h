#pragma once

#include "../core/splitter_system.h"
#include <httplib.h>
#include <json/json.h>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace web {

class RestServer {
public:
    RestServer(std::shared_ptr<core::SplitterSystem> splitter_system);
    ~RestServer();

    bool initialize(const std::string& bind_address = "0.0.0.0", int port = 8080);
    bool start();
    void stop();
    
    bool is_running() const { return running_.load(); }
    std::string get_bind_address() const { return bind_address_; }
    int get_port() const { return port_; }

private:
    std::shared_ptr<core::SplitterSystem> splitter_system_;
    std::unique_ptr<httplib::Server> http_server_;
    
    std::string bind_address_;
    int port_;
    
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    std::unique_ptr<std::thread> server_thread_;
    
    mutable std::mutex server_mutex_;
    
    // Server setup
    void setup_routes();
    void setup_cors();
    void setup_middleware();
    
    // Route handlers - System
    void handle_get_system_status(const httplib::Request& req, httplib::Response& res);
    void handle_get_system_info(const httplib::Request& req, httplib::Response& res);
    void handle_post_system_control(const httplib::Request& req, httplib::Response& res);
    void handle_post_system_calibrate(const httplib::Request& req, httplib::Response& res);
    void handle_post_system_test(const httplib::Request& req, httplib::Response& res);
    
    // Route handlers - Ports
    void handle_get_all_ports(const httplib::Request& req, httplib::Response& res);
    void handle_get_port(const httplib::Request& req, httplib::Response& res);
    void handle_post_port_control(const httplib::Request& req, httplib::Response& res);
    void handle_get_port_config(const httplib::Request& req, httplib::Response& res);
    void handle_put_port_config(const httplib::Request& req, httplib::Response& res);
    
    // Route handlers - WebSocket (for real-time updates)
    void handle_websocket(const httplib::Request& req, httplib::Response& res);
    
    // JSON conversion helpers
    Json::Value system_status_to_json(const core::SystemStatus& status) const;
    Json::Value port_status_to_json(const core::PortStatus& status) const;
    Json::Value system_info_to_json() const;
    
    // Utility functions
    void send_json_response(httplib::Response& res, const Json::Value& json, int status_code = 200);
    void send_error_response(httplib::Response& res, const std::string& message, int status_code = 400);
    bool validate_port_number(const std::string& port_str, int& port_number);
    void log_request(const httplib::Request& req);
    
    // Server thread function
    void server_thread_func();
};

} // namespace web