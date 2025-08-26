#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <libnetconf2/netconf.h>
#include <libnetconf2/session.h>

namespace splitter::core {
class SplitterManager;
}

namespace splitter::netconf {

class YangModel;

class NetconfServer {
public:
  explicit NetconfServer(core::SplitterManager *splitter_manager);
  ~NetconfServer();

  bool start(const std::string &host = "0.0.0.0", int port = 830);
  void stop();

  bool is_running() const { return server_running_; }
  std::string get_last_error() const { return last_error_; }

  bool load_yang_models(const std::vector<std::string> &model_paths);

private:
  struct ClientSession {
    struct nc_session *session{nullptr};
    std::thread session_thread;
    std::atomic<bool> active{false};
    std::string client_info;
  };

  void server_thread();
  void handle_client_session(std::shared_ptr<ClientSession> client);

  int handle_get_request(struct nc_session *session, const struct nc_rpc *rpc);
  int handle_get_config_request(struct nc_session *session,
                                const struct nc_rpc *rpc);
  int handle_edit_config_request(struct nc_session *session,
                                 const struct nc_rpc *rpc);
  int handle_commit_request(struct nc_session *session);
  int handle_copy_config_request(struct nc_session *session,
                                 const struct nc_rpc *rpc);

  std::string build_running_config_xml();
  std::string build_state_data_xml();
  std::string build_port_config_xml(int port_id);
  std::string build_port_state_xml(int port_id);

  bool apply_config_change(const std::string &config_xml);
  bool validate_config_xml(const std::string &config_xml);

  void cleanup_client_sessions();
  void send_notification(const std::string &notification_xml);

  core::SplitterManager *splitter_manager_;
  std::unique_ptr<YangModel> yang_model_;

  std::atomic<bool> server_running_{false};
  std::unique_ptr<std::thread> server_thread_;

  struct nc_server_endpt *endpoint_{nullptr};
  std::string bind_host_;
  int bind_port_{830};

  std::vector<std::shared_ptr<ClientSession>> client_sessions_;
  std::mutex sessions_mutex_;

  mutable std::string last_error_;

  static constexpr const char *YANG_MODULE_NAME = "splitter";
  static constexpr const char *YANG_NAMESPACE = "urn:splitter:yang:splitter";
  static constexpr const char *YANG_PREFIX = "spl";
};

} // namespace splitter::netconf