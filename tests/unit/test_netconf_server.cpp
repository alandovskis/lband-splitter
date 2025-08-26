#include <gtest/gtest.h>
#include "../../src/netconf/netconf_server.h"
#include "../../src/core/splitter_manager.h"
#include "../../src/core/config_manager.h"

namespace splitter::testing {

class NetconfServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_manager_ = std::make_unique<core::ConfigManager>("/tmp/test_config.json");
        splitter_manager_ = std::make_unique<core::SplitterManager>(config_manager_.get());
        netconf_server_ = std::make_unique<netconf::NetconfServer>(splitter_manager_.get());
    }
    
    void TearDown() override {
        if (netconf_server_) {
            netconf_server_->stop();
        }
        netconf_server_.reset();
        splitter_manager_.reset();
        config_manager_.reset();
    }
    
    std::unique_ptr<core::ConfigManager> config_manager_;
    std::unique_ptr<core::SplitterManager> splitter_manager_;
    std::unique_ptr<netconf::NetconfServer> netconf_server_;
};

TEST_F(NetconfServerTest, DefaultState) {
    EXPECT_FALSE(netconf_server_->is_running());
}

TEST_F(NetconfServerTest, LoadYangModels) {
    std::vector<std::string> empty_models;
    EXPECT_TRUE(netconf_server_->load_yang_models(empty_models));
}

} // namespace splitter::testing