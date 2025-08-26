#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <set>
#include <mutex>

#include <cpprest/ws_listener.h>
#include <cpprest/json.h>

namespace splitter::core {
    class SplitterManager;
}

namespace splitter::web {

class WebSocketServer {
public:
    explicit WebSocketServer(core::SplitterManager* splitter_manager, int port = 8081);
    ~WebSocketServer();
    
    bool start(const std::string& host = "0.0.0.0");
    void stop();
    
    bool is_running() const { return server_running_; }
    std::string get_last_error() const { return last_error_; }
    
    void broadcast_port_state_change(int port_id);
    void broadcast_frequency_change(int port_id, double frequency_mhz);
    void broadcast_system_stats();

private:
    void on_websocket_accept(web::websockets::experimental::listener::websocket_incoming_message msg);
    void on_websocket_close(web::websockets::experimental::listener::websocket_close_status close_status, 
                          const utility::string_t& reason);
    
    void handle_client_message(const std::string& message, 
                             std::shared_ptr<web::websockets::experimental::listener::websocket_callback_client> client);
    
    void broadcast_message(const web::json::value& message);
    void send_message_to_client(std::shared_ptr<web::websockets::experimental::listener::websocket_callback_client> client,
                               const web::json::value& message);
    
    web::json::value create_port_state_message(int port_id);
    web::json::value create_frequency_change_message(int port_id, double frequency_mhz);
    web::json::value create_system_stats_message();
    web::json::value create_welcome_message();
    
    core::SplitterManager* splitter_manager_;
    std::unique_ptr<web::websockets::experimental::listener::websocket_listener> ws_listener_;
    
    std::atomic<bool> server_running_{false};
    int port_;
    std::string host_;
    
    std::set<std::shared_ptr<web::websockets::experimental::listener::websocket_callback_client>> clients_;
    std::mutex clients_mutex_;
    
    mutable std::string last_error_;
};

} // namespace splitter::web