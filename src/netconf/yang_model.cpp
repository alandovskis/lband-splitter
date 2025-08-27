#include "yang_model.h"
#include "../utils/logger.h"

#include <filesystem>
#include <libyang/libyang.h>

namespace splitter::netconf {

YangModel::YangModel() = default;

YangModel::~YangModel() { cleanup_context(); }

bool YangModel::load_default_models() {
  if (!create_context()) {
    return false;
  }

  if (!load_ietf_models()) {
    last_error_ = "Failed to load IETF models";
    return false;
  }

  if (!load_splitter_model()) {
    last_error_ = "Failed to load splitter YANG model";
    return false;
  }

  utils::Logger::info("YANG models loaded successfully");
  return true;
}

bool YangModel::load_models(const std::vector<std::string> &model_paths) {
  if (!create_context()) {
    return false;
  }

  for (const auto &path : model_paths) {
    if (!std::filesystem::exists(path)) {
      last_error_ = "YANG model file not found: " + path;
      return false;
    }

    const struct lys_module *module =
        lys_parse_path(context_, path.c_str(), LYS_IN_YANG);
    if (!module) {
      last_error_ = "Failed to parse YANG model: " + path;
      return false;
    }

    utils::Logger::debug("Loaded YANG model: {}", path);
  }

  return true;
}

bool YangModel::load_model_from_string(const std::string &yang_content) {
  if (!context_) {
    if (!create_context()) {
      return false;
    }
  }

  const struct lys_module *module =
      lys_parse_mem(context_, yang_content.c_str(), LYS_IN_YANG);
  if (!module) {
    last_error_ = "Failed to parse YANG model from string";
    return false;
  }

  return true;
}

bool YangModel::validate_config(const std::string &config_xml) {
  if (!context_) {
    last_error_ = "YANG context not initialized";
    return false;
  }

  struct lyd_node *data_tree = nullptr;

  if (lyd_parse_data_mem(context_, config_xml.c_str(), LYD_XML,
                         LYD_PARSE_STRICT | LYD_PARSE_NO_STATE,
                         LYD_VALIDATE_PRESENT, &data_tree) != LY_SUCCESS) {
    last_error_ = "Failed to parse configuration XML";
    if (data_tree) {
      lyd_free_tree(data_tree);
    }
    return false;
  }

  if (lyd_validate_all(&data_tree, context_, LYD_VALIDATE_PRESENT, nullptr) !=
      LY_SUCCESS) {
    last_error_ = "Configuration validation failed";
    lyd_free_tree(data_tree);
    return false;
  }

  lyd_free_tree(data_tree);
  return true;
}

bool YangModel::validate_data(const std::string &data_xml) {
  if (!context_) {
    last_error_ = "YANG context not initialized";
    return false;
  }

  struct lyd_node *data_tree = nullptr;

  if (lyd_parse_data_mem(context_, data_xml.c_str(), LYD_XML, LYD_PARSE_STRICT,
                         LYD_VALIDATE_PRESENT, &data_tree) != LY_SUCCESS) {
    last_error_ = "Failed to parse data XML";
    if (data_tree) {
      lyd_free_tree(data_tree);
    }
    return false;
  }

  if (lyd_validate_all(&data_tree, context_, LYD_VALIDATE_PRESENT, nullptr) !=
      LY_SUCCESS) {
    last_error_ = "Data validation failed";
    lyd_free_tree(data_tree);
    return false;
  }

  lyd_free_tree(data_tree);
  return true;
}

bool YangModel::create_context() {
  if (context_) {
    return true;
  }

  if (ly_ctx_new(nullptr, LY_CTX_NO_YANGLIBRARY, &context_) != LY_SUCCESS) {
    last_error_ = "Failed to create libyang context";
    return false;
  }

  auto search_paths = get_yang_search_paths();
  for (const auto &path : search_paths) {
    if (std::filesystem::exists(path)) {
      ly_ctx_set_searchdir(context_, path.c_str());
    }
  }

  return true;
}

void YangModel::cleanup_context() {
  if (context_) {
    ly_ctx_destroy(context_);
    context_ = nullptr;
    splitter_module_ = nullptr;
  }
}

bool YangModel::load_ietf_models() {
  const std::vector<std::string> ietf_models = {
      "ietf-yang-types", "ietf-inet-types", "ietf-interfaces", "ietf-system"};

  for (const auto &model_name : ietf_models) {
    const struct lys_module *module =
        ly_ctx_load_module(context_, model_name.c_str(), nullptr, nullptr);
    if (!module) {
      utils::Logger::warning("Failed to load IETF model: {}", model_name);
    } else {
      utils::Logger::debug("Loaded IETF model: {}", model_name);
    }
  }

  return true;
}

bool YangModel::load_splitter_model() {
  std::string yang_content = get_default_splitter_yang();

  splitter_module_ = lys_parse_mem(context_, yang_content.c_str(), LYS_IN_YANG);
  if (!splitter_module_) {
    last_error_ = "Failed to load splitter YANG model";
    return false;
  }

  utils::Logger::info("Loaded splitter YANG model");
  return true;
}

std::string YangModel::get_default_splitter_yang() {
  return R"(
module splitter {
    namespace "urn:splitter:yang:splitter";
    prefix "spl";
    
    import ietf-yang-types {
        prefix yang;
    }
    
    import ietf-inet-types {
        prefix inet;
    }
    
    organization "L-band Splitter Systems";
    contact "support@splitter.example.com";
    description "YANG model for 32-port L-band splitter/combiner system";
    
    revision "2024-01-01" {
        description "Initial revision";
    }
    
    container splitter {
        description "L-band splitter/combiner system configuration and state";
        
        container system {
            description "System-wide configuration and state";
            
            leaf name {
                type string;
                config false;
                description "System name";
            }
            
            leaf description {
                type string;
                config false;
                description "System description";
            }
            
            container status {
                config false;
                description "System operational status";
                
                leaf active-ports {
                    type uint32;
                    description "Number of currently active ports";
                }
                
                leaf hardware-healthy {
                    type boolean;
                    description "Overall hardware health status";
                }
                
                leaf total-state-changes {
                    type uint64;
                    description "Total number of port state changes since startup";
                }
                
                leaf uptime {
                    type uint64;
                    units "seconds";
                    description "System uptime in seconds";
                }
            }
        }
        
        container ports {
            description "Port configuration and state information";
            
            list port {
                key "id";
                description "Individual port configuration and state";
                
                leaf id {
                    type uint8 {
                        range "0..31";
                    }
                    description "Port identifier (0-31)";
}

leaf name {
  type string;
  description "Human-readable port name";
}

leaf auto - enable {
  type boolean;
  default false;
  description "Enable port automatically on system startup";
}

leaf min - frequency {
  type decimal64 {
    fraction - digits 1;
    range "950.0..2150.0";
  }
  units "MHz";
  default 1000.0;
  description "Minimum acceptable frequency for signal detection";
}

leaf max - frequency {
  type decimal64 {
    fraction - digits 1;
    range "950.0..2150.0";
  }
  units "MHz";
  default 2000.0;
  description "Maximum acceptable frequency for signal detection";
}

leaf gain {
  type int8 { range "-30..30"; }
  units "dB";
  default 0;
  description "Port gain adjustment";
}

leaf signal - detection - enabled {
  type boolean;
  default true;
  description "Enable automatic signal detection for this port";
}

// State data (config false)

leaf enabled {
  type boolean;
  config false;
  description "Current port enable status";
}

leaf signal - detected {
  type boolean;
  config false;
  description "Whether a valid signal is detected on this port";
}

leaf healthy {
  type boolean;
  config false;
  description "Port hardware health status";
}

leaf frequency {
  type decimal64 { fraction - digits 1; }
  units "MHz";
  config false;
  description "Current detected frequency";
}

leaf signal - level {
  type decimal64 { fraction - digits 1; }
  units "dBm";
  config false;
  description "Current signal level";
}

leaf error {
  type string;
  config false;
  description "Current error message, if any";
}

leaf last-update {
  type yang:date-and-time;
  config false;
  description "Timestamp of last port state update";
}
}
}
}

// RPC operations

rpc enable - port {
  description "Enable a specific port";

  input {
    leaf port - id {
      type uint8 { range "0..31"; }
      mandatory true;
      description "Port to enable";
    }
  }

  output {
    leaf result {
      type boolean;
      description "Operation result";
    }

    leaf message {
      type string;
      description "Result message";
    }
  }
}

rpc disable - port {
  description "Disable a specific port";

  input {
    leaf port - id {
      type uint8 { range "0..31"; }
      mandatory true;
      description "Port to disable";
    }
  }

  output {
    leaf result {
      type boolean;
      description "Operation result";
    }

    leaf message {
      type string;
      description "Result message";
    }
  }
}

// Notifications

notification port - state - change {
  description "Notification sent when a port changes state";

  leaf port - id {
    type uint8 { range "0..31"; }
    description "Port that changed state";
  }

  leaf enabled {
    type boolean;
    description "New enable status";
  }

  leaf signal - detected {
    type boolean;
    description "New signal detection status";
  }

  leaf healthy {
    type boolean;
    description "New health status";
  }

  leaf timestamp {
    type yang : date - and-time;
    description "Time of state change";
  }
}

notification frequency - change {
  description "Notification sent when port frequency changes significantly";

  leaf port - id {
    type uint8 { range "0..31"; }
    description "Port with frequency change";
  }

  leaf frequency {
    type decimal64 { fraction - digits 1; }
    units "MHz";
    description "New detected frequency";
  }

  leaf signal - level {
    type decimal64 { fraction - digits 1; }
    units "dBm";
    description "Signal level";
  }

  leaf timestamp {
    type yang:date-and-time;
    description "Time of frequency change";
  }
}
}
    )";
    }

    std::vector<std::string> YangModel::get_yang_search_paths() {
      return {"/usr/share/splitter/yang",
              "/usr/local/share/splitter/yang",
              "/usr/share/yang/modules",
              "/usr/local/share/yang/modules",
              "config/yang",
              "."};
    }

    } // namespace splitter::netconf