#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../src/core/config_manager.h"
#include "../../src/core/splitter_manager.h"

using ::testing::_;

namespace splitter::testing {

class SplitterManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    config_manager_ =
        std::make_unique<core::ConfigManager>("/tmp/test_config.json");
    splitter_manager_ =
        std::make_unique<core::SplitterManager>(config_manager_.get());
  }

  void TearDown() override {
    if (splitter_manager_) {
      splitter_manager_->shutdown();
    }
    splitter_manager_.reset();
    config_manager_.reset();
    std::remove("/tmp/test_config.json");
  }

  std::unique_ptr<core::ConfigManager> config_manager_;
  std::unique_ptr<core::SplitterManager> splitter_manager_;
};

TEST_F(SplitterManagerTest, InvalidPortOperations) {
  EXPECT_FALSE(splitter_manager_->enable_port(-1));
  EXPECT_FALSE(splitter_manager_->enable_port(32));
  EXPECT_FALSE(splitter_manager_->disable_port(-1));
  EXPECT_FALSE(splitter_manager_->disable_port(32));
}

TEST_F(SplitterManagerTest, GetAllPortStates) {
  auto states = splitter_manager_->get_all_port_states();
  EXPECT_EQ(states.size(), core::SplitterManager::NUM_PORTS);

  for (int i = 0; i < core::SplitterManager::NUM_PORTS; ++i) {
    EXPECT_EQ(states[i].id, i);
  }
}

TEST_F(SplitterManagerTest, PortConfiguration) {
  core::PortConfig config;
  config.name = "Test Port";
  config.min_frequency_mhz = 1200.0;
  config.max_frequency_mhz = 1800.0;

  EXPECT_TRUE(splitter_manager_->set_port_configuration(0, config));

  auto retrieved_config = splitter_manager_->get_port_configuration(0);
  EXPECT_EQ(retrieved_config.name, "Test Port");
}

TEST_F(SplitterManagerTest, SystemStats) {
  auto stats = splitter_manager_->get_system_stats();
  EXPECT_EQ(stats.active_ports, 0);
  EXPECT_GE(stats.total_state_changes, 0);
  EXPECT_GE(stats.total_frequency_updates, 0);
}

TEST_F(SplitterManagerTest, CallbackRegistration) {
  bool state_callback_called = false;
  bool frequency_callback_called = false;

  splitter_manager_->register_state_change_callback(
      [&](int port_id, const core::PortState &state) {
        state_callback_called = true;
      });

  splitter_manager_->register_frequency_change_callback(
      [&](int port_id, double frequency) { frequency_callback_called = true; });

  splitter_manager_->process_events();
}

} // namespace splitter::testing