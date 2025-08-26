#include <gtest/gtest.h>
#include "../../src/utils/system_monitor.h"
#include "../../src/core/splitter_manager.h"
#include "../../src/core/config_manager.h"

namespace splitter::testing {

class SystemMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_manager_ = std::make_unique<core::ConfigManager>("/tmp/test_config.json");
        splitter_manager_ = std::make_unique<core::SplitterManager>(config_manager_.get());
        system_monitor_ = std::make_unique<utils::SystemMonitor>(splitter_manager_.get());
    }
    
    void TearDown() override {
        if (system_monitor_) {
            system_monitor_->stop();
        }
        system_monitor_.reset();
        splitter_manager_.reset();
        config_manager_.reset();
    }
    
    std::unique_ptr<core::ConfigManager> config_manager_;
    std::unique_ptr<core::SplitterManager> splitter_manager_;
    std::unique_ptr<utils::SystemMonitor> system_monitor_;
};

TEST_F(SystemMonitorTest, DefaultState) {
    EXPECT_FALSE(system_monitor_->is_running());
}

TEST_F(SystemMonitorTest, GetSystemMetrics) {
    auto metrics = system_monitor_->get_system_metrics();
    EXPECT_GE(metrics.cpu_usage_percent, 0.0);
    EXPECT_GE(metrics.memory_usage_percent, 0.0);
}

TEST_F(SystemMonitorTest, GetApplicationMetrics) {
    auto metrics = system_monitor_->get_application_metrics();
    EXPECT_EQ(metrics.active_ports, 0);
    EXPECT_GE(metrics.total_state_changes, 0);
}

} // namespace splitter::testing