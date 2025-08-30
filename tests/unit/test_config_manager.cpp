#include <filesystem>
#include <gtest/gtest.h>

#include "../../src/core/config_manager.h"

namespace splitter::testing {

class ConfigManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_config_file_ = "/tmp/test_splitter_config.json";
    config_manager_ = std::make_unique<core::ConfigManager>(test_config_file_);
  }

  void TearDown() override {
    config_manager_.reset();
    if (std::filesystem::exists(test_config_file_)) {
      std::filesystem::remove(test_config_file_);
    }
  }

  std::string test_config_file_;
  std::unique_ptr<core::ConfigManager> config_manager_;
};

TEST_F(ConfigManagerTest, LoadDefaultConfig) {
  EXPECT_TRUE(config_manager_->load_config());
  EXPECT_TRUE(config_manager_->validate_config());
}

TEST_F(ConfigManagerTest, NetworkConfiguration) {
  ASSERT_TRUE(config_manager_->load_config());

  core::NetworkConfig net_config;
  net_config.rest_host = "127.0.0.1";
  net_config.rest_port = 9090;
  net_config.netconf_port = 831;

  EXPECT_TRUE(config_manager_->set_network_config(net_config));

  auto retrieved = config_manager_->get_network_config();
  EXPECT_EQ(retrieved.rest_host, "127.0.0.1");
  EXPECT_EQ(retrieved.rest_port, 9090);
  EXPECT_EQ(retrieved.netconf_port, 831);
}

// Hardware configuration test removed - STM32 handles all hardware
// TEST_F(ConfigManagerTest, HardwareConfiguration) { ... }

TEST_F(ConfigManagerTest, LoggingConfiguration) {
  ASSERT_TRUE(config_manager_->load_config());

  core::LoggingConfig log_config;
  log_config.log_file = "/tmp/test.log";
  log_config.log_level = "debug";
  log_config.max_file_size_mb = 50;
  log_config.max_files = 5;

  EXPECT_TRUE(config_manager_->set_logging_config(log_config));

  auto retrieved = config_manager_->get_logging_config();
  EXPECT_EQ(retrieved.log_file, "/tmp/test.log");
  EXPECT_EQ(retrieved.log_level, "debug");
  EXPECT_EQ(retrieved.max_file_size_mb, 50);
  EXPECT_EQ(retrieved.max_files, 5);
}

TEST_F(ConfigManagerTest, MonitoringConfiguration) {
  ASSERT_TRUE(config_manager_->load_config());

  core::MonitoringConfig mon_config;
  mon_config.metrics_interval_seconds = 60;
  mon_config.enable_health_endpoint = false;

  EXPECT_TRUE(config_manager_->set_monitoring_config(mon_config));

  auto retrieved = config_manager_->get_monitoring_config();
  EXPECT_EQ(retrieved.metrics_interval_seconds, 60);
  EXPECT_EQ(retrieved.enable_health_endpoint, false);
}

TEST_F(ConfigManagerTest, InvalidConfiguration) {
  ASSERT_TRUE(config_manager_->load_config());

  core::NetworkConfig invalid_config;
  invalid_config.rest_port = -1;

  EXPECT_FALSE(config_manager_->set_network_config(invalid_config));
}

TEST_F(ConfigManagerTest, SaveAndLoadConfig) {
  ASSERT_TRUE(config_manager_->load_config());

  core::NetworkConfig net_config;
  net_config.rest_port = 8888;
  EXPECT_TRUE(config_manager_->set_network_config(net_config));

  EXPECT_TRUE(config_manager_->save_config());

  auto new_config_manager =
      std::make_unique<core::ConfigManager>(test_config_file_);
  EXPECT_TRUE(new_config_manager->load_config());

  auto retrieved = new_config_manager->get_network_config();
  EXPECT_EQ(retrieved.rest_port, 8888);
}

TEST_F(ConfigManagerTest, YangModelPaths) {
  auto model_path = config_manager_->get_yang_model_path();
  auto search_paths = config_manager_->get_yang_search_paths();

  EXPECT_FALSE(search_paths.empty());

  bool has_standard_paths = false;
  for (const auto &path : search_paths) {
    if (path.find("/usr/share") != std::string::npos ||
        path.find("config/yang") != std::string::npos) {
      has_standard_paths = true;
      break;
    }
  }
  EXPECT_TRUE(has_standard_paths);
}

} // namespace splitter::testing