#include "websocket_server.h"
#include "../core/splitter_manager.h"
#include "../utils/logger.h"

namespace splitter::web {

WebSocketServer::WebSocketServer(core::SplitterManager* splitter_manager, int port)
    : splitter_manager_(splitter_manager), port_(port) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start(const std::string& host) {
    if (server_running_) {
        utils::Logger::warning("WebSocket server already running");
        return true;
    }
    
    try {
        host_ = host;
        
        web::http::uri_builder uri_builder;
        uri_builder.set_scheme("ws");
        uri_builder.set_host(host);
        uri_builder.set_port(port_);
        
        ws_listener_ = std::make_unique<web::websockets::experimental::listener::websocket_listener>(uri_builder.to_uri());
        
        ws_listener_->set_message_handler([this](web::websockets::experimental::listener::websocket_incoming_message msg) {
            on_websocket_accept(std::move(msg));
        });
        
        ws_listener_->set_close_handler([this](web::websockets::experimental::listener::websocket_close_status close_status, 
                                              const utility::string_t& reason, 
                                              const std::error_code& error) {
            on_websocket_close(close_status, reason);
        });
        
        ws_listener_->open().wait();
        
        server_running_ = true;
        utils::Logger::info("WebSocket server started on {}:{}", host, port_);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Failed to start WebSocket server: " + std::string(e.what());
        utils::Logger::error("WebSocket server start failed: {}", e.what());
        return false;
    }
}

void WebSocketServer::stop() {
    if (!server_running_) {
        return;
    }
    
    server_running_ = false;
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& client : clients_) {
            try {
                if (client) {
                    client->close().wait();
                }
            } catch (const std::exception& e) {
                utils::Logger::warning("Exception closing WebSocket client: {}", e.what());
            }
        }
        clients_.clear();
    }
    
    if (ws_listener_) {
        ws_listener_->close().wait();
        ws_listener_.reset();
    }
    
    utils::Logger::info("WebSocket server stopped");
}

void WebSocketServer::on_websocket_accept(web::websockets::experimental::listener::websocket_incoming_message msg) {
    try {
        auto client = msg.get_listener();
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_.insert(client);
        }
        
        send_message_to_client(client, create_welcome_message());
        
        utils::Logger::debug("New WebSocket client connected, total clients: {}", clients_.size());
        
        msg.get_body().then([this, client](std::string body) {
            handle_client_message(body, client);
        });
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception handling WebSocket accept: {}", e.what());
    }
}

void WebSocketServer::on_websocket_close(web::websockets::experimental::listener::websocket_close_status close_status, 
                                       const utility::string_t& reason) {
    utils::Logger::debug("WebSocket client disconnected: {}", utility::conversions::to_utf8string(reason));
}

void WebSocketServer::handle_client_message(const std::string& message, 
                                           std::shared_ptr<web::websockets::experimental::listener::websocket_callback_client> client) {
    try {
        web::json::value request = web::json::value::parse(message);
        
        if (!request.has_field("type") || !request["type"].is_string()) {
            return;
        }
        
        std::string msg_type = request["type"].as_string();
        
        if (msg_type == "subscribe_all") {
            send_message_to_client(client, create_system_stats_message());
            
            for (int i = 0; i < core::SplitterManager::NUM_PORTS; ++i) {
                send_message_to_client(client, create_port_state_message(i));
            }
        } else if (msg_type == "subscribe_port") {
            if (request.has_field("port_id") && request["port_id"].is_number()) {
                int port_id = request["port_id"].as_integer();
                if (port_id >= 0 && port_id < core::SplitterManager::NUM_PORTS) {
                    send_message_to_client(client, create_port_state_message(port_id));
                }
            }
        } else if (msg_type == "ping") {
            web::json::value pong;
            pong["type"] = web::json::value::string("pong");
            pong["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
            send_message_to_client(client, pong);
        }
        
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception handling WebSocket message: {}", e.what());
    }
}

void WebSocketServer::broadcast_port_state_change(int port_id) {
    if (!server_running_ || port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
        return;
    }
    
    try {
        auto message = create_port_state_message(port_id);
        broadcast_message(message);
        
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception broadcasting port state change: {}", e.what());
    }
}

void WebSocketServer::broadcast_frequency_change(int port_id, double frequency_mhz) {
    if (!server_running_ || port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
        return;
    }
    
    try {
        auto message = create_frequency_change_message(port_id, frequency_mhz);
        broadcast_message(message);
        
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception broadcasting frequency change: {}", e.what());
    }
}

void WebSocketServer::broadcast_system_stats() {
    if (!server_running_) {
        return;
    }
    
    try {
        auto message = create_system_stats_message();
        broadcast_message(message);
        
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception broadcasting system stats: {}", e.what());
    }
}

void WebSocketServer::broadcast_message(const web::json::value& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = clients_.begin();
    while (it != clients_.end()) {
        try {
            if (*it) {
                (*it)->send(message).wait();
                ++it;
            } else {
                it = clients_.erase(it);
            }
        } catch (const std::exception& e) {
            utils::Logger::warning("Exception sending WebSocket message, removing client: {}", e.what());
            it = clients_.erase(it);
        }
    }
}

void WebSocketServer::send_message_to_client(std::shared_ptr<web::websockets::experimental::listener::websocket_callback_client> client,
                                            const web::json::value& message) {
    try {
        if (client) {
            client->send(message).wait();
        }
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception sending WebSocket message to client: {}", e.what());
        
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(client);
    }
}

web::json::value WebSocketServer::create_port_state_message(int port_id) {
    web::json::value message;
    message["type"] = web::json::value::string("port_state");
    message["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
    
    if (splitter_manager_) {
        auto state = splitter_manager_->get_port_state(port_id);
        auto config = splitter_manager_->get_port_configuration(port_id);
        
        web::json::value port_data;
        port_data["id"] = web::json::value::number(port_id);
        port_data["name"] = web::json::value::string(config.name);
        port_data["enabled"] = web::json::value::boolean(state.enabled);
        port_data["signal_detected"] = web::json::value::boolean(state.signal_detected);
        port_data["healthy"] = web::json::value::boolean(state.healthy);
        port_data["frequency_mhz"] = web::json::value::number(state.frequency_mhz);
        port_data["signal_level_dbm"] = web::json::value::number(state.signal_level_dbm);
        port_data["error_message"] = web::json::value::string(state.error_message);
        
        message["port"] = port_data;
    }
    
    return message;
}

web::json::value WebSocketServer::create_frequency_change_message(int port_id, double frequency_mhz) {
    web::json::value message;
    message["type"] = web::json::value::string("frequency_change");
    message["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
    message["port_id"] = web::json::value::number(port_id);
    message["frequency_mhz"] = web::json::value::number(frequency_mhz);
    
    if (splitter_manager_) {
        auto state = splitter_manager_->get_port_state(port_id);
        message["signal_level_dbm"] = web::json::value::number(state.signal_level_dbm);
    }
    
    return message;
}

web::json::value WebSocketServer::create_system_stats_message() {
    web::json::value message;
    message["type"] = web::json::value::string("system_stats");
    message["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
    
    if (splitter_manager_) {
        auto stats = splitter_manager_->get_system_stats();
        
        web::json::value stats_data;
        stats_data["active_ports"] = web::json::value::number(stats.active_ports);
        stats_data["total_state_changes"] = web::json::value::number(stats.total_state_changes);
        stats_data["total_frequency_updates"] = web::json::value::number(stats.total_frequency_updates);
        stats_data["average_frequency_mhz"] = web::json::value::number(stats.average_frequency_mhz);
        stats_data["hardware_healthy"] = web::json::value::boolean(stats.hardware_healthy);
        
        message["stats"] = stats_data;
    }
    
    return message;
}

web::json::value WebSocketServer::create_welcome_message() {
    web::json::value message;
    message["type"] = web::json::value::string("welcome");
    message["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
    message["server"] = web::json::value::string("L-band Splitter WebSocket Server");
    message["version"] = web::json::value::string("1.0.0");
    
    return message;
}

} // namespace splitter::web