#pragma once

#include "../core/splitter_system.h"
#include <libnetconf2/netconf.h>
#include <libyang/libyang.h>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace netconf {

class NetConfServer {
public:
    NetConfServer(std::shared_ptr<core::SplitterSystem> splitter_system);
    ~NetConfServer();

    bool initialize(const std::string& bind_address = "0.0.0.0", uint16_t port = 830);
    bool start();
    void stop();
    
    bool is_running() const { return running_.load(); }

private:
    std::shared_ptr<core::SplitterSystem> splitter_system_;
    
    struct nc_server* server_;
    const struct ly_ctx* ly_ctx_;
    
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    std::unique_ptr<std::thread> server_thread_;
    
    mutable std::mutex server_mutex_;
    
    // Server management
    void server_loop();
    bool setup_yang_models();
    bool configure_server();
    
    // NETCONF callbacks
    static int get_config_callback(struct lyd_node **config, 
                                   NC_DATASTORE source,
                                   const char *running_xpath,
                                   void *user_data);
    
    static int edit_config_callback(const struct lyd_node *config,
                                    NC_DATASTORE target,
                                    const char *running_xpath,
                                    void *user_data);
    
    static int rpc_callback(struct lyd_node *rpc_tree,
                           struct nc_session *session,
                           void *user_data);
    
    // Data conversion helpers
    struct lyd_node* create_system_config() const;
    struct lyd_node* create_system_state() const;
    struct lyd_node* create_port_config(int port_number) const;
    struct lyd_node* create_port_state(int port_number) const;
    
    bool apply_system_config(const struct lyd_node* config);
    bool apply_port_config(int port_number, const struct lyd_node* config);
    
    // RPC handlers
    struct lyd_node* handle_enable_port_rpc(const struct lyd_node* input);
    struct lyd_node* handle_disable_port_rpc(const struct lyd_node* input);
    struct lyd_node* handle_enable_all_ports_rpc();
    struct lyd_node* handle_disable_all_ports_rpc();
    struct lyd_node* handle_self_test_rpc();
    struct lyd_node* handle_calibrate_rpc();
    
    // Utility functions
    std::string get_iso_timestamp() const;
    void send_notification(const std::string& notification_name, 
                          const std::map<std::string, std::string>& data);
    
    // Status change callback from splitter system
    void on_system_status_change(const core::SystemStatus& status);
};

} // namespace netconf