#pragma once

#include <memory>
#include <atomic>
#include <string>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

namespace splitter::core {
    class SplitterManager;
}

namespace splitter::web {

class WebSocketServer;

class RestServer {
public:
    explicit RestServer(core::SplitterManager* splitter_manager, int port = 8080);
    ~RestServer();
    
    bool start(const std::string& host = "0.0.0.0");
    void stop();
    
    bool is_running() const { return server_running_; }
    std::string get_last_error() const { return last_error_; }

private:
    void handle_get_request(web::http::http_request request);
    void handle_post_request(web::http::http_request request);
    void handle_put_request(web::http::http_request request);
    void handle_delete_request(web::http::http_request request);
    void handle_options_request(web::http::http_request request);
    
    // API endpoint handlers
    void get_system_status(web::http::http_request request);
    void get_system_metrics(web::http::http_request request);
    void get_all_ports(web::http::http_request request);
    void get_port_info(web::http::http_request request, int port_id);
    void get_port_state(web::http::http_request request, int port_id);
    void get_port_config(web::http::http_request request, int port_id);
    
    void enable_port(web::http::http_request request, int port_id);
    void disable_port(web::http::http_request request, int port_id);
    void set_port_config(web::http::http_request request, int port_id);
    
    void get_health_check(web::http::http_request request);
    
    // Response helpers
    void send_json_response(web::http::http_request request, 
                          const web::json::value& json, 
                          web::http::status_code status = web::http::status_codes::OK);
    void send_error_response(web::http::http_request request, 
                           web::http::status_code status, 
                           const std::string& message);
    void add_cors_headers(web::http::http_response& response);
    
    // JSON conversion helpers
    web::json::value port_state_to_json(const core::PortState& state, const core::PortConfig& config);
    web::json::value system_stats_to_json(const core::SystemStats& stats);
    core::PortConfig json_to_port_config(const web::json::value& json);
    
    bool parse_port_id_from_path(const std::string& path, int& port_id);
    
    core::SplitterManager* splitter_manager_;
    std::unique_ptr<web::http::experimental::listener::http_listener> http_listener_;
    std::unique_ptr<WebSocketServer> websocket_server_;
    
    std::atomic<bool> server_running_{false};
    int port_;
    std::string host_;
    
    mutable std::string last_error_;
};

} // namespace splitter::web