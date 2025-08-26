#include "rest_server.h"
#include "websocket_server.h"
#include "../core/splitter_manager.h"
#include "../utils/logger.h"

#include <regex>

namespace splitter::web {

RestServer::RestServer(core::SplitterManager* splitter_manager, int port)
    : splitter_manager_(splitter_manager), port_(port) {
}

RestServer::~RestServer() {
    stop();
}

bool RestServer::start(const std::string& host) {
    if (server_running_) {
        utils::Logger::warning("REST server already running");
        return true;
    }
    
    try {
        host_ = host;
        
        web::http::uri_builder uri_builder;
        uri_builder.set_scheme("http");
        uri_builder.set_host(host);
        uri_builder.set_port(port_);
        
        http_listener_ = std::make_unique<web::http::experimental::listener::http_listener>(uri_builder.to_uri());
        
        http_listener_->support(web::http::methods::GET, 
            [this](web::http::http_request request) { handle_get_request(std::move(request)); });
        http_listener_->support(web::http::methods::POST, 
            [this](web::http::http_request request) { handle_post_request(std::move(request)); });
        http_listener_->support(web::http::methods::PUT, 
            [this](web::http::http_request request) { handle_put_request(std::move(request)); });
        http_listener_->support(web::http::methods::DEL, 
            [this](web::http::http_request request) { handle_delete_request(std::move(request)); });
        http_listener_->support(web::http::methods::OPTIONS, 
            [this](web::http::http_request request) { handle_options_request(std::move(request)); });
        
        http_listener_->open().wait();
        
        websocket_server_ = std::make_unique<WebSocketServer>(splitter_manager_, port_ + 1);
        if (!websocket_server_->start(host)) {
            utils::Logger::warning("Failed to start WebSocket server");
        }
        
        server_running_ = true;
        utils::Logger::info("REST API server started on {}:{}", host, port_);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Failed to start REST server: " + std::string(e.what());
        utils::Logger::error("REST server start failed: {}", e.what());
        return false;
    }
}

void RestServer::stop() {
    if (!server_running_) {
        return;
    }
    
    server_running_ = false;
    
    if (websocket_server_) {
        websocket_server_->stop();
        websocket_server_.reset();
    }
    
    if (http_listener_) {
        http_listener_->close().wait();
        http_listener_.reset();
    }
    
    utils::Logger::info("REST API server stopped");
}

void RestServer::handle_get_request(web::http::http_request request) {
    auto path = request.relative_uri().path();
    
    if (path == "/api/v1/system/status") {
        get_system_status(std::move(request));
    } else if (path == "/api/v1/system/metrics") {
        get_system_metrics(std::move(request));
    } else if (path == "/api/v1/ports") {
        get_all_ports(std::move(request));
    } else if (path == "/health") {
        get_health_check(std::move(request));
    } else {
        int port_id;
        if (parse_port_id_from_path(path, port_id)) {
            if (path.find("/state") != std::string::npos) {
                get_port_state(std::move(request), port_id);
            } else if (path.find("/config") != std::string::npos) {
                get_port_config(std::move(request), port_id);
            } else {
                get_port_info(std::move(request), port_id);
            }
        } else {
            send_error_response(std::move(request), web::http::status_codes::NotFound, "Endpoint not found");
        }
    }
}

void RestServer::handle_post_request(web::http::http_request request) {
    auto path = request.relative_uri().path();
    
    int port_id;
    if (parse_port_id_from_path(path, port_id)) {
        if (path.find("/enable") != std::string::npos) {
            enable_port(std::move(request), port_id);
        } else if (path.find("/disable") != std::string::npos) {
            disable_port(std::move(request), port_id);
        } else {
            send_error_response(std::move(request), web::http::status_codes::NotFound, "Endpoint not found");
        }
    } else {
        send_error_response(std::move(request), web::http::status_codes::NotFound, "Endpoint not found");
    }
}

void RestServer::handle_put_request(web::http::http_request request) {
    auto path = request.relative_uri().path();
    
    int port_id;
    if (parse_port_id_from_path(path, port_id) && path.find("/config") != std::string::npos) {
        set_port_config(std::move(request), port_id);
    } else {
        send_error_response(std::move(request), web::http::status_codes::NotFound, "Endpoint not found");
    }
}

void RestServer::handle_delete_request(web::http::http_request request) {
    send_error_response(std::move(request), web::http::status_codes::MethodNotAllowed, "DELETE method not supported");
}

void RestServer::handle_options_request(web::http::http_request request) {
    web::http::http_response response(web::http::status_codes::OK);
    add_cors_headers(response);
    response.headers().add("Allow", "GET, POST, PUT, OPTIONS");
    request.reply(response);
}

void RestServer::get_system_status(web::http::http_request request) {
    try {
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        auto stats = splitter_manager_->get_system_stats();
        bool healthy = splitter_manager_->get_system_health();
        
        web::json::value status;
        status["healthy"] = web::json::value::boolean(healthy);
        status["active_ports"] = web::json::value::number(stats.active_ports);
        status["total_ports"] = web::json::value::number(core::SplitterManager::NUM_PORTS);
        status["hardware_healthy"] = web::json::value::boolean(stats.hardware_healthy);
        status["uptime"] = web::json::value::string("running");
        
        send_json_response(std::move(request), status);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_system_status: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::get_system_metrics(web::http::http_request request) {
    try {
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        auto stats = splitter_manager_->get_system_stats();
        auto metrics = system_stats_to_json(stats);
        
        send_json_response(std::move(request), metrics);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_system_metrics: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::get_all_ports(web::http::http_request request) {
    try {
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        web::json::value ports_array = web::json::value::array();
        
        for (int i = 0; i < core::SplitterManager::NUM_PORTS; ++i) {
            auto state = splitter_manager_->get_port_state(i);
            auto config = splitter_manager_->get_port_configuration(i);
            
            ports_array[i] = port_state_to_json(state, config);
        }
        
        web::json::value response;
        response["ports"] = ports_array;
        
        send_json_response(std::move(request), response);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_all_ports: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::get_port_info(web::http::http_request request, int port_id) {
    try {
        if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
            send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
            return;
        }
        
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        auto state = splitter_manager_->get_port_state(port_id);
        auto config = splitter_manager_->get_port_configuration(port_id);
        
        auto port_json = port_state_to_json(state, config);
        send_json_response(std::move(request), port_json);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_port_info: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::get_port_state(web::http::http_request request, int port_id) {
    try {
        if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
            send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
            return;
        }
        
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        auto state = splitter_manager_->get_port_state(port_id);
        
        web::json::value state_json;
        state_json["id"] = web::json::value::number(state.id);
        state_json["enabled"] = web::json::value::boolean(state.enabled);
        state_json["signal_detected"] = web::json::value::boolean(state.signal_detected);
        state_json["healthy"] = web::json::value::boolean(state.healthy);
        state_json["frequency_mhz"] = web::json::value::number(state.frequency_mhz);
        state_json["signal_level_dbm"] = web::json::value::number(state.signal_level_dbm);
        state_json["error_message"] = web::json::value::string(state.error_message);
        
        send_json_response(std::move(request), state_json);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_port_state: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::get_port_config(web::http::http_request request, int port_id) {
    try {
        if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
            send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
            return;
        }
        
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        auto config = splitter_manager_->get_port_configuration(port_id);
        
        web::json::value config_json;
        config_json["name"] = web::json::value::string(config.name);
        config_json["auto_enable"] = web::json::value::boolean(config.auto_enable);
        config_json["min_frequency_mhz"] = web::json::value::number(config.min_frequency_mhz);
        config_json["max_frequency_mhz"] = web::json::value::number(config.max_frequency_mhz);
        config_json["gain_db"] = web::json::value::number(config.gain_db);
        config_json["signal_detection_enabled"] = web::json::value::boolean(config.signal_detection_enabled);
        
        send_json_response(std::move(request), config_json);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_port_config: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::enable_port(web::http::http_request request, int port_id) {
    try {
        if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
            send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
            return;
        }
        
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        bool success = splitter_manager_->enable_port(port_id);
        
        web::json::value response;
        response["success"] = web::json::value::boolean(success);
        response["port_id"] = web::json::value::number(port_id);
        response["message"] = web::json::value::string(success ? "Port enabled successfully" : "Failed to enable port");
        
        auto status = success ? web::http::status_codes::OK : web::http::status_codes::InternalError;
        send_json_response(std::move(request), response, status);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in enable_port: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::disable_port(web::http::http_request request, int port_id) {
    try {
        if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
            send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
            return;
        }
        
        if (!splitter_manager_) {
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
            return;
        }
        
        bool success = splitter_manager_->disable_port(port_id);
        
        web::json::value response;
        response["success"] = web::json::value::boolean(success);
        response["port_id"] = web::json::value::number(port_id);
        response["message"] = web::json::value::string(success ? "Port disabled successfully" : "Failed to disable port");
        
        auto status = success ? web::http::status_codes::OK : web::http::status_codes::InternalError;
        send_json_response(std::move(request), response, status);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in disable_port: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::set_port_config(web::http::http_request request, int port_id) {
    request.extract_json().then([=](web::json::value body) {
        try {
            if (port_id < 0 || port_id >= core::SplitterManager::NUM_PORTS) {
                send_error_response(std::move(request), web::http::status_codes::BadRequest, "Invalid port ID");
                return;
            }
            
            if (!splitter_manager_) {
                send_error_response(std::move(request), web::http::status_codes::InternalError, "Splitter manager not available");
                return;
            }
            
            auto config = json_to_port_config(body);
            bool success = splitter_manager_->set_port_configuration(port_id, config);
            
            web::json::value response;
            response["success"] = web::json::value::boolean(success);
            response["port_id"] = web::json::value::number(port_id);
            response["message"] = web::json::value::string(success ? "Port configuration updated successfully" : "Failed to update port configuration");
            
            auto status = success ? web::http::status_codes::OK : web::http::status_codes::InternalError;
            send_json_response(std::move(request), response, status);
            
        } catch (const std::exception& e) {
            utils::Logger::error("Exception in set_port_config: {}", e.what());
            send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
        }
    });
}

void RestServer::get_health_check(web::http::http_request request) {
    try {
        bool healthy = splitter_manager_ ? splitter_manager_->get_system_health() : false;
        
        web::json::value health;
        health["status"] = web::json::value::string(healthy ? "healthy" : "unhealthy");
        health["timestamp"] = web::json::value::string("2024-01-01T00:00:00Z");
        
        auto status = healthy ? web::http::status_codes::OK : web::http::status_codes::ServiceUnavailable;
        send_json_response(std::move(request), health, status);
        
    } catch (const std::exception& e) {
        utils::Logger::error("Exception in get_health_check: {}", e.what());
        send_error_response(std::move(request), web::http::status_codes::InternalError, "Internal server error");
    }
}

void RestServer::send_json_response(web::http::http_request request, 
                                  const web::json::value& json, 
                                  web::http::status_code status) {
    web::http::http_response response(status);
    add_cors_headers(response);
    response.headers().add("Content-Type", "application/json");
    response.set_body(json);
    request.reply(response);
}

void RestServer::send_error_response(web::http::http_request request, 
                                   web::http::status_code status, 
                                   const std::string& message) {
    web::json::value error;
    error["error"] = web::json::value::string(message);
    error["status_code"] = web::json::value::number(static_cast<int>(status));
    
    send_json_response(std::move(request), error, status);
}

void RestServer::add_cors_headers(web::http::http_response& response) {
    response.headers().add("Access-Control-Allow-Origin", "*");
    response.headers().add("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response.headers().add("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

web::json::value RestServer::port_state_to_json(const core::PortState& state, const core::PortConfig& config) {
    web::json::value port;
    
    port["id"] = web::json::value::number(state.id);
    port["name"] = web::json::value::string(config.name);
    port["enabled"] = web::json::value::boolean(state.enabled);
    port["signal_detected"] = web::json::value::boolean(state.signal_detected);
    port["healthy"] = web::json::value::boolean(state.healthy);
    port["frequency_mhz"] = web::json::value::number(state.frequency_mhz);
    port["signal_level_dbm"] = web::json::value::number(state.signal_level_dbm);
    port["error_message"] = web::json::value::string(state.error_message);
    
    web::json::value config_json;
    config_json["auto_enable"] = web::json::value::boolean(config.auto_enable);
    config_json["min_frequency_mhz"] = web::json::value::number(config.min_frequency_mhz);
    config_json["max_frequency_mhz"] = web::json::value::number(config.max_frequency_mhz);
    config_json["gain_db"] = web::json::value::number(config.gain_db);
    config_json["signal_detection_enabled"] = web::json::value::boolean(config.signal_detection_enabled);
    
    port["config"] = config_json;
    
    return port;
}

web::json::value RestServer::system_stats_to_json(const core::SystemStats& stats) {
    web::json::value metrics;
    
    metrics["active_ports"] = web::json::value::number(stats.active_ports);
    metrics["total_state_changes"] = web::json::value::number(stats.total_state_changes);
    metrics["total_frequency_updates"] = web::json::value::number(stats.total_frequency_updates);
    metrics["average_frequency_mhz"] = web::json::value::number(stats.average_frequency_mhz);
    metrics["hardware_healthy"] = web::json::value::boolean(stats.hardware_healthy);
    
    return metrics;
}

core::PortConfig RestServer::json_to_port_config(const web::json::value& json) {
    core::PortConfig config;
    
    if (json.has_field("name") && json["name"].is_string()) {
        config.name = json["name"].as_string();
    }
    
    if (json.has_field("auto_enable") && json["auto_enable"].is_boolean()) {
        config.auto_enable = json["auto_enable"].as_bool();
    }
    
    if (json.has_field("min_frequency_mhz") && json["min_frequency_mhz"].is_number()) {
        config.min_frequency_mhz = json["min_frequency_mhz"].as_double();
    }
    
    if (json.has_field("max_frequency_mhz") && json["max_frequency_mhz"].is_number()) {
        config.max_frequency_mhz = json["max_frequency_mhz"].as_double();
    }
    
    if (json.has_field("gain_db") && json["gain_db"].is_number()) {
        config.gain_db = json["gain_db"].as_integer();
    }
    
    if (json.has_field("signal_detection_enabled") && json["signal_detection_enabled"].is_boolean()) {
        config.signal_detection_enabled = json["signal_detection_enabled"].as_bool();
    }
    
    return config;
}

bool RestServer::parse_port_id_from_path(const std::string& path, int& port_id) {
    std::regex port_regex(R"(/api/v1/ports/(\d+)(?:/.*)?$)");
    std::smatch matches;
    
    if (std::regex_match(path, matches, port_regex)) {
        try {
            port_id = std::stoi(matches[1].str());
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    return false;
}

} // namespace splitter::web