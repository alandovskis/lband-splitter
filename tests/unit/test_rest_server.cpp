#include <gtest/gtest.h>
#include "../../src/web/rest_server.h"
#include "../../src/core/splitter_manager.h"
#include "../../src/core/config_manager.h"

namespace splitter::testing {

class RestServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_manager_ = std::make_unique<core::ConfigManager>("/tmp/test_config.json");
        splitter_manager_ = std::make_unique<core::SplitterManager>(config_manager_.get());
        rest_server_ = std::make_unique<web::RestServer>(splitter_manager_.get(), 18080);
    }
    
    void TearDown() override {
        if (rest_server_) {
            rest_server_->stop();
        }
        rest_server_.reset();
        splitter_manager_.reset();
        config_manager_.reset();
    }
    
    std::unique_ptr<core::ConfigManager> config_manager_;
    std::unique_ptr<core::SplitterManager> splitter_manager_;
    std::unique_ptr<web::RestServer> rest_server_;
};

TEST_F(RestServerTest, DefaultState) {
    EXPECT_FALSE(rest_server_->is_running());
}

} // namespace splitter::testing