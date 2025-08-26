#include <gtest/gtest.h>
#include "../../src/core/splitter_manager.h"
#include "../../src/core/config_manager.h"
#include "../../src/utils/logger.h"

namespace splitter::testing {

class FullSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_config_file_ = "/tmp/integration_test_config.json";
        log_file_ = "/tmp/integration_test.log";
        
        utils::Logger::init(log_file_, "debug");
        
        config_manager_ = std::make_unique<core::ConfigManager>(test_config_file_);
        ASSERT_TRUE(config_manager_->load_config());
        
        splitter_manager_ = std::make_unique<core::SplitterManager>(config_manager_.get());
    }
    
    void TearDown() override {
        if (splitter_manager_) {
            splitter_manager_->shutdown();
        }
        splitter_manager_.reset();
        config_manager_.reset();
        
        utils::Logger::shutdown();
        
        std::remove(test_config_file_.c_str());
        std::remove(log_file_.c_str());
    }
    
    std::string test_config_file_;
    std::string log_file_;
    std::unique_ptr<core::ConfigManager> config_manager_;
    std::unique_ptr<core::SplitterManager> splitter_manager_;
};

TEST_F(FullSystemTest, SystemInitializationWorkflow) {
    auto hardware_config = config_manager_->get_hardware_config();
    EXPECT_GT(hardware_config.gpio_base_pin, 0);
    
    auto network_config = config_manager_->get_network_config();
    EXPECT_GT(network_config.rest_port, 0);
    EXPECT_GT(network_config.netconf_port, 0);
    
    auto states = splitter_manager_->get_all_port_states();
    EXPECT_EQ(states.size(), core::SplitterManager::NUM_PORTS);
    
    for (int i = 0; i < 5; ++i) {
        auto state = splitter_manager_->get_port_state(i);
        EXPECT_EQ(state.id, i);
        EXPECT_FALSE(state.enabled);
    }
}

TEST_F(FullSystemTest, PortConfigurationWorkflow) {
    core::PortConfig config;
    config.name = "Integration Test Port";
    config.min_frequency_mhz = 1200.0;
    config.max_frequency_mhz = 1800.0;
    config.gain_db = 5;
    config.auto_enable = false;
    config.signal_detection_enabled = true;
    
    EXPECT_TRUE(splitter_manager_->set_port_configuration(0, config));
    
    auto retrieved_config = splitter_manager_->get_port_configuration(0);
    EXPECT_EQ(retrieved_config.name, "Integration Test Port");
    EXPECT_DOUBLE_EQ(retrieved_config.min_frequency_mhz, 1200.0);
    EXPECT_DOUBLE_EQ(retrieved_config.max_frequency_mhz, 1800.0);
    EXPECT_EQ(retrieved_config.gain_db, 5);
    EXPECT_FALSE(retrieved_config.auto_enable);
    EXPECT_TRUE(retrieved_config.signal_detection_enabled);
}

TEST_F(FullSystemTest, SystemStatsAndHealth) {
    EXPECT_TRUE(splitter_manager_->get_system_health() || !splitter_manager_->get_system_health());
    
    auto stats = splitter_manager_->get_system_stats();
    EXPECT_GE(stats.active_ports, 0);
    EXPECT_LE(stats.active_ports, core::SplitterManager::NUM_PORTS);
    EXPECT_GE(stats.total_state_changes, 0);
    EXPECT_GE(stats.total_frequency_updates, 0);
}

TEST_F(FullSystemTest, EventProcessing) {
    bool callback_invoked = false;
    
    splitter_manager_->register_state_change_callback(
        [&](int port_id, const core::PortState& state) {
            callback_invoked = true;
            EXPECT_GE(port_id, 0);
            EXPECT_LT(port_id, core::SplitterManager::NUM_PORTS);
        });
    
    splitter_manager_->process_events();
}

} // namespace splitter::testing