#include "rest_server.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace web {

RestServer::RestServer(std::shared_ptr<core::SplitterSystem> splitter_system)
    : splitter_system_(splitter_system)
    , http_server_(std::make_unique<httplib::Server>())
    , port_(8080)
    , running_(false)
    , initialized_(false) {
}

RestServer::~RestServer() {
    stop();
}

bool RestServer::initialize(const std::string& bind_address, int port) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    
    if (initialized_.load()) {
        return true;
    }
    
    bind_address_ = bind_address;
    port_ = port;
    
    std::cout << "Initializing REST server on " << bind_address_ << ":" << port_ << std::endl;
    
    if (!http_server_) {
        std::cerr << "Failed to create HTTP server instance" << std::endl;
        return false;
    }
    
    setup_cors();
    setup_middleware();
    setup_routes();
    
    initialized_.store(true);
    std::cout << "REST server initialized successfully" << std::endl;
    
    return true;
}

bool RestServer::start() {
    std::lock_guard<std::mutex> lock(server_mutex_);
    
    if (!initialized_.load() || running_.load()) {
        return false;
    }
    
    std::cout << "Starting REST server..." << std::endl;
    
    running_.store(true);
    server_thread_ = std::make_unique<std::thread>(&RestServer::server_thread_func, this);
    
    return true;
}

void RestServer::stop() {
    std::cout << "Stopping REST server..." << std::endl;
    
    running_.store(false);
    
    {
        std::lock_guard<std::mutex> lock(server_mutex_);
        if (http_server_) {
            http_server_->stop();
        }
    }
    
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    initialized_.store(false);
    std::cout << "REST server stopped" << std::endl;
}

void RestServer::setup_routes() {
    // System routes
    http_server_->Get("/api/v1/system/status", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_system_status(req, res);
        });
    
    http_server_->Get("/api/v1/system/info", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_system_info(req, res);
        });
    
    http_server_->Post("/api/v1/system/control", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_post_system_control(req, res);
        });
    
    http_server_->Post("/api/v1/system/calibrate", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_post_system_calibrate(req, res);
        });
    
    http_server_->Post("/api/v1/system/test", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_post_system_test(req, res);
        });
    
    // Port routes
    http_server_->Get("/api/v1/ports", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_all_ports(req, res);
        });
    
    http_server_->Get(R"(/api/v1/ports/(\d+))", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_port(req, res);
        });
    
    http_server_->Post(R"(/api/v1/ports/(\d+)/control)", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_post_port_control(req, res);
        });
    
    http_server_->Get(R"(/api/v1/ports/(\d+)/config)", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_port_config(req, res);
        });
    
    http_server_->Put(R"(/api/v1/ports/(\d+)/config)", 
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_put_port_config(req, res);
        });
    
    // Static file serving for Angular app
    http_server_->set_mount_point("/", "./webapp/dist/");
    
    // Default route - serve Angular app
    http_server_->Get("/.*", [this](const httplib::Request& req, httplib::Response& res) {
        // Serve index.html for Angular routing
        if (req.path.find("/api/") == std::string::npos) {
            res.set_content_type("text/html");
            // In a real implementation, read and serve index.html
            res.set_content("<html><body><h1>L-Band Splitter Control</h1><p>Angular app would load here</p></body></html>");
        }
    });
}

void RestServer::setup_cors() {
    // Enable CORS for development
    http_server_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });
    
    // Handle OPTIONS requests for CORS
    http_server_->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return;
    });
}

void RestServer::setup_middleware() {
    // Request logging
    http_server_->set_logger([this](const httplib::Request& req, const httplib::Response& res) {
        log_request(req);
    });
    
    // Error handler
    http_server_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        Json::Value error;
        error["error"] = "Internal server error";
        error["status"] = res.status;
        
        Json::StreamWriterBuilder builder;
        std::string json_string = Json::writeString(builder, error);
        
        res.set_content(json_string, "application/json");
    });
}

void RestServer::handle_get_system_status(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    auto status = splitter_system_->get_system_status();
    auto json = system_status_to_json(status);
    
    send_json_response(res, json);
}

void RestServer::handle_get_system_info(const httplib::Request& req, httplib::Response& res) {
    auto json = system_info_to_json();
    send_json_response(res, json);
}

void RestServer::handle_post_system_control(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    Json::Value request_json;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream iss(req.body);
    if (!Json::parseFromStream(builder, iss, &request_json, &errors)) {
        send_error_response(res, "Invalid JSON: " + errors, 400);
        return;
    }
    
    if (!request_json.isMember("action")) {
        send_error_response(res, "Missing 'action' field", 400);
        return;
    }
    
    std::string action = request_json["action"].asString();
    bool success = false;
    
    if (action == "enable_all") {
        success = splitter_system_->enable_all_ports();
    } else if (action == "disable_all") {
        success = splitter_system_->disable_all_ports();
    } else {
        send_error_response(res, "Unknown action: " + action, 400);
        return;
    }
    
    Json::Value response;
    response["success"] = success;
    response["message"] = success ? "Operation completed successfully" : "Operation failed";
    
    send_json_response(res, response);
}

void RestServer::handle_post_system_calibrate(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    bool success = splitter_system_->calibrate_frequency_detectors();
    
    Json::Value response;
    response["success"] = success;
    response["message"] = success ? "Calibration completed successfully" : "Calibration failed";
    
    send_json_response(res, response);
}

void RestServer::handle_post_system_test(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    bool success = splitter_system_->run_self_test();
    
    Json::Value response;
    response["success"] = success;
    response["message"] = success ? "Self-test passed" : "Self-test failed";
    
    send_json_response(res, response);
}

void RestServer::handle_get_all_ports(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    auto statuses = splitter_system_->get_all_port_statuses();
    
    Json::Value json_array(Json::arrayValue);
    for (const auto& status : statuses) {
        json_array.append(port_status_to_json(status));
    }
    
    send_json_response(res, json_array);
}

void RestServer::handle_get_port(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    int port_number;
    if (!validate_port_number(req.matches[1], port_number)) {
        send_error_response(res, "Invalid port number", 400);
        return;
    }
    
    auto status = splitter_system_->get_port_status(port_number);
    auto json = port_status_to_json(status);
    
    send_json_response(res, json);
}

void RestServer::handle_post_port_control(const httplib::Request& req, httplib::Response& res) {
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    int port_number;
    if (!validate_port_number(req.matches[1], port_number)) {
        send_error_response(res, "Invalid port number", 400);
        return;
    }
    
    Json::Value request_json;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream iss(req.body);
    if (!Json::parseFromStream(builder, iss, &request_json, &errors)) {
        send_error_response(res, "Invalid JSON: " + errors, 400);
        return;
    }
    
    if (!request_json.isMember("action")) {
        send_error_response(res, "Missing 'action' field", 400);
        return;
    }
    
    std::string action = request_json["action"].asString();
    bool success = false;
    
    if (action == "enable") {
        success = splitter_system_->enable_port(port_number);
    } else if (action == "disable") {
        success = splitter_system_->disable_port(port_number);
    } else {
        send_error_response(res, "Unknown action: " + action, 400);
        return;
    }
    
    Json::Value response;
    response["success"] = success;
    response["message"] = success ? "Port operation completed successfully" : "Port operation failed";
    response["port"] = port_number;
    
    send_json_response(res, response);
}

void RestServer::handle_get_port_config(const httplib::Request& req, httplib::Response& res) {
    int port_number;
    if (!validate_port_number(req.matches[1], port_number)) {
        send_error_response(res, "Invalid port number", 400);
        return;
    }
    
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    auto config = splitter_system_->get_port_configuration(port_number);
    
    Json::Value json;
    json["port"] = port_number;
    for (const auto& pair : config) {
        json["config"][pair.first] = pair.second;
    }
    
    send_json_response(res, json);
}

void RestServer::handle_put_port_config(const httplib::Request& req, httplib::Response& res) {
    int port_number;
    if (!validate_port_number(req.matches[1], port_number)) {
        send_error_response(res, "Invalid port number", 400);
        return;
    }
    
    if (!splitter_system_) {
        send_error_response(res, "Splitter system not available", 500);
        return;
    }
    
    Json::Value request_json;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream iss(req.body);
    if (!Json::parseFromStream(builder, iss, &request_json, &errors)) {
        send_error_response(res, "Invalid JSON: " + errors, 400);
        return;
    }
    
    std::map<std::string, std::string> config;
    if (request_json.isMember("config") && request_json["config"].isObject()) {
        for (const auto& key : request_json["config"].getMemberNames()) {
            config[key] = request_json["config"][key].asString();
        }
    }
    
    bool success = splitter_system_->set_port_configuration(port_number, config);
    
    Json::Value response;
    response["success"] = success;
    response["message"] = success ? "Port configuration updated successfully" : "Port configuration update failed";
    response["port"] = port_number;
    
    send_json_response(res, response);
}

Json::Value RestServer::system_status_to_json(const core::SystemStatus& status) const {
    Json::Value json;
    
    json["enabled_ports"] = status.enabled_ports;
    json["active_ports"] = status.active_ports;
    json["system_healthy"] = status.system_healthy;
    json["system_error"] = status.system_error;
    
    // Convert timestamp
    auto time_t = std::chrono::system_clock::to_time_t(status.last_update);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["last_update"] = oss.str();
    
    Json::Value ports_array(Json::arrayValue);
    for (const auto& port_status : status.port_statuses) {
        ports_array.append(port_status_to_json(port_status));
    }
    json["ports"] = ports_array;
    
    return json;
}

Json::Value RestServer::port_status_to_json(const core::PortStatus& status) const {
    Json::Value json;
    
    json["port_number"] = status.port_number;
    
    // Convert port state enum to string
    switch (status.state) {
        case core::PortState::DISABLED:
            json["state"] = "disabled";
            break;
        case core::PortState::ENABLED:
            json["state"] = "enabled";
            break;
        case core::PortState::ERROR:
            json["state"] = "error";
            break;
    }
    
    json["signal_detected"] = status.signal_detected;
    json["center_frequency_mhz"] = status.center_frequency_mhz;
    json["power_level_dbm"] = status.power_level_dbm;
    json["error_message"] = status.error_message;
    
    // Convert timestamp
    auto time_t = std::chrono::system_clock::to_time_t(status.last_update);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    json["last_update"] = oss.str();
    
    return json;
}

Json::Value RestServer::system_info_to_json() const {
    Json::Value json;
    
    json["system_name"] = "L-Band Splitter/Combiner";
    json["hardware_version"] = "1.0";
    json["software_version"] = "1.0.0";
    json["num_ports"] = core::SplitterSystem::NUM_PORTS;
    json["api_version"] = "v1";
    
    // Calculate uptime (simplified)
    json["uptime_seconds"] = 0; // Would be calculated from system start time
    
    return json;
}

void RestServer::send_json_response(httplib::Response& res, const Json::Value& json, int status_code) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::string json_string = Json::writeString(builder, json);
    
    res.status = status_code;
    res.set_content(json_string, "application/json");
}

void RestServer::send_error_response(httplib::Response& res, const std::string& message, int status_code) {
    Json::Value error;
    error["error"] = message;
    error["status"] = status_code;
    
    send_json_response(res, error, status_code);
}

bool RestServer::validate_port_number(const std::string& port_str, int& port_number) {
    try {
        port_number = std::stoi(port_str);
        return port_number >= 0 && port_number < core::SplitterSystem::NUM_PORTS;
    } catch (const std::exception&) {
        return false;
    }
}

void RestServer::log_request(const httplib::Request& req) {
    std::cout << "[REST] " << req.method << " " << req.path;
    if (!req.body.empty()) {
        std::cout << " (body: " << req.body.length() << " bytes)";
    }
    std::cout << std::endl;
}

void RestServer::server_thread_func() {
    std::cout << "REST server listening on " << bind_address_ << ":" << port_ << std::endl;
    
    if (!http_server_->listen(bind_address_, port_)) {
        std::cerr << "Failed to start HTTP server on " << bind_address_ << ":" << port_ << std::endl;
        running_.store(false);
    }
}

} // namespace web