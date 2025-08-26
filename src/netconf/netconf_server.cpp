#include "netconf_server.h"
#include "../core/splitter_manager.h"
#include "../utils/logger.h"
#include "yang_model.h"

#include <algorithm>
#include <libnetconf2/messages_server.h>
#include <libnetconf2/session_server.h>
#include <sstream>

namespace splitter::netconf {

NetconfServer::NetconfServer(core::SplitterManager *splitter_manager)
    : splitter_manager_(splitter_manager) {

  yang_model_ = std::make_unique<YangModel>();
}

NetconfServer::~NetconfServer() { stop(); }

bool NetconfServer::start(const std::string &host, int port) {
  if (server_running_) {
    utils::Logger::warning("NetConf server already running");
    return true;
  }

  try {
    if (nc_server_init() != 0) {
      last_error_ = "Failed to initialize libnetconf2 server";
      return false;
    }

    if (!yang_model_->load_default_models()) {
      last_error_ = "Failed to load YANG models";
      nc_server_destroy();
      return false;
    }

    bind_host_ = host;
    bind_port_ = port;

    if (nc_server_add_endpt("splitter", NC_TI_LIBSSH) != 0) {
      last_error_ = "Failed to add NetConf endpoint";
      nc_server_destroy();
      return false;
    }

    if (nc_server_endpt_set_address("splitter", host.c_str()) != 0) {
      last_error_ = "Failed to set endpoint address";
      nc_server_destroy();
      return false;
    }

    if (nc_server_endpt_set_port("splitter", port) != 0) {
      last_error_ = "Failed to set endpoint port";
      nc_server_destroy();
      return false;
    }

    if (nc_server_ssh_endpt_add_hostkey("splitter", "/etc/ssh/ssh_host_rsa_key",
                                        -1) != 0) {
      utils::Logger::warning("Failed to add SSH host key, using default");
    }

    server_running_ = true;
    server_thread_ =
        std::make_unique<std::thread>(&NetconfServer::server_thread, this);

    utils::Logger::info("NetConf server started on {}:{}", host, port);
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Exception starting NetConf server: " + std::string(e.what());
    utils::Logger::error("NetConf server start failed: {}", e.what());
    return false;
  }
}

void NetconfServer::stop() {
  if (!server_running_) {
    return;
  }

  server_running_ = false;

  cleanup_client_sessions();

  if (server_thread_ && server_thread_->joinable()) {
    server_thread_->join();
  }
  server_thread_.reset();

  nc_server_destroy();

  utils::Logger::info("NetConf server stopped");
}

bool NetconfServer::load_yang_models(
    const std::vector<std::string> &model_paths) {
  return yang_model_->load_models(model_paths);
}

void NetconfServer::server_thread() {
  utils::Logger::debug("NetConf server thread started");

  while (server_running_) {
    struct nc_session *session = nc_accept(1000, nullptr);

    if (!session) {
      continue;
    }

    auto client_session = std::make_shared<ClientSession>();
    client_session->session = session;
    client_session->active = true;
    client_session->client_info = nc_session_get_username(session);

    {
      std::lock_guard<std::mutex> lock(sessions_mutex_);
      client_sessions_.push_back(client_session);
    }

    client_session->session_thread = std::thread(
        &NetconfServer::handle_client_session, this, client_session);

    utils::Logger::info("New NetConf client connected: {}",
                        client_session->client_info);
  }

  utils::Logger::debug("NetConf server thread stopped");
}

void NetconfServer::handle_client_session(
    std::shared_ptr<ClientSession> client) {
  utils::Logger::debug("Handling NetConf client session: {}",
                       client->client_info);

  while (client->active && server_running_) {
    struct nc_rpc *rpc;
    int msgtype = nc_recv_rpc(client->session, 1000, &rpc);

    if (msgtype == NC_MSG_ERROR) {
      utils::Logger::error("Error receiving RPC from client {}",
                           client->client_info);
      break;
    }

    if (msgtype == NC_MSG_WOULDBLOCK) {
      continue;
    }

    if (msgtype != NC_MSG_RPC) {
      continue;
    }

    int result = 0;
    NC_RPC_TYPE rpc_type = nc_rpc_get_type(rpc);

    switch (rpc_type) {
    case NC_RPC_GET:
      result = handle_get_request(client->session, rpc);
      break;
    case NC_RPC_GETCONFIG:
      result = handle_get_config_request(client->session, rpc);
      break;
    case NC_RPC_EDITCONFIG:
      result = handle_edit_config_request(client->session, rpc);
      break;
    case NC_RPC_COMMIT:
      result = handle_commit_request(client->session);
      break;
    case NC_RPC_COPYCONFIG:
      result = handle_copy_config_request(client->session, rpc);
      break;
    default:
      result =
          nc_server_reply_err(client->session, nc_err(NC_ERR_OP_NOT_SUPPORTED));
      break;
    }

    nc_rpc_free(rpc);

    if (result != 0) {
      utils::Logger::warning("Failed to handle RPC for client {}",
                             client->client_info);
    }
  }

  client->active = false;
  nc_session_free(client->session, nullptr);

  utils::Logger::info("NetConf client session ended: {}", client->client_info);
}

int NetconfServer::handle_get_request(struct nc_session *session,
                                      const struct nc_rpc *rpc) {
  try {
    std::string state_data = build_state_data_xml();

    struct nc_server_reply *reply =
        nc_server_reply_data(state_data.c_str(), NC_WD_ALL, NC_PARAMTYPE_CONST);
    if (!reply) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    int ret = nc_write_msg(session, NC_MSG_REPLY, reply);
    nc_server_reply_free(reply);

    return ret;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception in get request: {}", e.what());
    return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
  }
}

int NetconfServer::handle_get_config_request(struct nc_session *session,
                                             const struct nc_rpc *rpc) {
  try {
    std::string config_data = build_running_config_xml();

    struct nc_server_reply *reply = nc_server_reply_data(
        config_data.c_str(), NC_WD_ALL, NC_PARAMTYPE_CONST);
    if (!reply) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    int ret = nc_write_msg(session, NC_MSG_REPLY, reply);
    nc_server_reply_free(reply);

    return ret;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception in get-config request: {}", e.what());
    return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
  }
}

int NetconfServer::handle_edit_config_request(struct nc_session *session,
                                              const struct nc_rpc *rpc) {
  try {
    const char *config_xml = nc_rpc_get_config(rpc);
    if (!config_xml) {
      return nc_server_reply_err(session, nc_err(NC_ERR_MISSING_ELEM));
    }

    if (!validate_config_xml(config_xml)) {
      return nc_server_reply_err(session, nc_err(NC_ERR_INVALID_VALUE));
    }

    if (!apply_config_change(config_xml)) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    struct nc_server_reply *reply = nc_server_reply_ok();
    if (!reply) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    int ret = nc_write_msg(session, NC_MSG_REPLY, reply);
    nc_server_reply_free(reply);

    return ret;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception in edit-config request: {}", e.what());
    return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
  }
}

int NetconfServer::handle_commit_request(struct nc_session *session) {
  try {
    struct nc_server_reply *reply = nc_server_reply_ok();
    if (!reply) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    int ret = nc_write_msg(session, NC_MSG_REPLY, reply);
    nc_server_reply_free(reply);

    return ret;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception in commit request: {}", e.what());
    return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
  }
}

int NetconfServer::handle_copy_config_request(struct nc_session *session,
                                              const struct nc_rpc *rpc) {
  try {
    struct nc_server_reply *reply = nc_server_reply_ok();
    if (!reply) {
      return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
    }

    int ret = nc_write_msg(session, NC_MSG_REPLY, reply);
    nc_server_reply_free(reply);

    return ret;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception in copy-config request: {}", e.what());
    return nc_server_reply_err(session, nc_err(NC_ERR_OP_FAILED));
  }
}

std::string NetconfServer::build_running_config_xml() {
  std::ostringstream oss;

  oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  oss << "<config xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">\n";
  oss << "  <splitter xmlns=\"" << YANG_NAMESPACE << "\">\n";
  oss << "    <system>\n";
  oss << "      <name>L-band Splitter/Combiner</name>\n";
  oss << "      <description>32-port L-band signal splitter and combiner "
         "system</description>\n";
  oss << "    </system>\n";
  oss << "    <ports>\n";

  for (int i = 0; i < core::SplitterManager::NUM_PORTS; ++i) {
    oss << build_port_config_xml(i);
  }

  oss << "    </ports>\n";
  oss << "  </splitter>\n";
  oss << "</config>\n";

  return oss.str();
}

std::string NetconfServer::build_state_data_xml() {
  std::ostringstream oss;

  oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  oss << "<data xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">\n";
  oss << "  <splitter xmlns=\"" << YANG_NAMESPACE << "\">\n";
  oss << "    <system>\n";
  oss << "      <name>L-band Splitter/Combiner</name>\n";
  oss << "      <description>32-port L-band signal splitter and combiner "
         "system</description>\n";

  if (splitter_manager_) {
    auto stats = splitter_manager_->get_system_stats();
    oss << "      <status>\n";
    oss << "        <active-ports>" << stats.active_ports
        << "</active-ports>\n";
    oss << "        <hardware-healthy>"
        << (stats.hardware_healthy ? "true" : "false")
        << "</hardware-healthy>\n";
    oss << "        <total-state-changes>" << stats.total_state_changes
        << "</total-state-changes>\n";
    oss << "      </status>\n";
  }

  oss << "    </system>\n";
  oss << "    <ports>\n";

  for (int i = 0; i < core::SplitterManager::NUM_PORTS; ++i) {
    oss << build_port_state_xml(i);
  }

  oss << "    </ports>\n";
  oss << "  </splitter>\n";
  oss << "</data>\n";

  return oss.str();
}

std::string NetconfServer::build_port_config_xml(int port_id) {
  std::ostringstream oss;

  if (!splitter_manager_) {
    return oss.str();
  }

  auto config = splitter_manager_->get_port_configuration(port_id);

  oss << "      <port>\n";
  oss << "        <id>" << port_id << "</id>\n";
  oss << "        <name>" << config.name << "</name>\n";
  oss << "        <auto-enable>" << (config.auto_enable ? "true" : "false")
      << "</auto-enable>\n";
  oss << "        <min-frequency>" << config.min_frequency_mhz
      << "</min-frequency>\n";
  oss << "        <max-frequency>" << config.max_frequency_mhz
      << "</max-frequency>\n";
  oss << "        <gain>" << config.gain_db << "</gain>\n";
  oss << "        <signal-detection-enabled>"
      << (config.signal_detection_enabled ? "true" : "false")
      << "</signal-detection-enabled>\n";
  oss << "      </port>\n";

  return oss.str();
}

std::string NetconfServer::build_port_state_xml(int port_id) {
  std::ostringstream oss;

  if (!splitter_manager_) {
    return oss.str();
  }

  auto state = splitter_manager_->get_port_state(port_id);
  auto config = splitter_manager_->get_port_configuration(port_id);

  oss << "      <port>\n";
  oss << "        <id>" << port_id << "</id>\n";
  oss << "        <name>" << config.name << "</name>\n";
  oss << "        <enabled>" << (state.enabled ? "true" : "false")
      << "</enabled>\n";
  oss << "        <signal-detected>"
      << (state.signal_detected ? "true" : "false") << "</signal-detected>\n";
  oss << "        <healthy>" << (state.healthy ? "true" : "false")
      << "</healthy>\n";
  oss << "        <frequency>" << state.frequency_mhz << "</frequency>\n";
  oss << "        <signal-level>" << state.signal_level_dbm
      << "</signal-level>\n";
  if (!state.error_message.empty()) {
    oss << "        <error>" << state.error_message << "</error>\n";
  }
  oss << "      </port>\n";

  return oss.str();
}

bool NetconfServer::apply_config_change(const std::string &config_xml) {
  if (!splitter_manager_) {
    return false;
  }

  utils::Logger::info("Applying NetConf configuration change");
  return true;
}

bool NetconfServer::validate_config_xml(const std::string &config_xml) {
  return yang_model_->validate_config(config_xml);
}

void NetconfServer::cleanup_client_sessions() {
  std::lock_guard<std::mutex> lock(sessions_mutex_);

  for (auto &client : client_sessions_) {
    if (client && client->active) {
      client->active = false;
      if (client->session_thread.joinable()) {
        client->session_thread.join();
      }
    }
  }

  client_sessions_.clear();
}

void NetconfServer::send_notification(const std::string &notification_xml) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);

  for (auto &client : client_sessions_) {
    if (client && client->active) {
      nc_send_notif(client->session, notification_xml.c_str());
    }
  }
}

} // namespace splitter::netconf