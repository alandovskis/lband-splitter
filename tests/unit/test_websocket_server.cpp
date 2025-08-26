#include <gtest/gtest.h>
#include "../../src/web/websocket_server.h"
#include "../../src/core/splitter_manager.h"
#include "../../src/core/config_manager.h"

namespace splitter::testing {

class WebSocketServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_manager_ = std::make_unique<core::ConfigManager>("/tmp/test_config.json");
        splitter_manager_ = std::make_unique<core::SplitterManager>(config_manager_.get());
        ws_server_ = std::make_unique<web::WebSocketServer>(splitter_manager_.get(), 18081);
    }
    
    void TearDown() override {
        if (ws_server_) {
            ws_server_->stop();
        }
        ws_server_.reset();
        splitter_manager_.reset();
        config_manager_.reset();
    }
    
    std::unique_ptr<core::ConfigManager> config_manager_;
    std::unique_ptr<core::SplitterManager> splitter_manager_;
    std::unique_ptr<web::WebSocketServer> ws_server_;
};

TEST_F(WebSocketServerTest, DefaultState) {
    EXPECT_FALSE(ws_server_->is_running());
}

} // namespace splitter::testing