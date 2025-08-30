#include "../../src/hardware/stm32f4_controller.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace splitter::testing {

class MockUARTInterface {
public:
  MOCK_METHOD(bool, initialize, (uint32_t baud_rate), ());
  MOCK_METHOD(void, cleanup, (), ());
  MOCK_METHOD(bool, send_data, (const uint8_t *data, size_t length), ());
  MOCK_METHOD(bool, receive_data,
              (uint8_t *data, size_t max_length, size_t &received_length,
               uint32_t timeout_ms),
              ());
  MOCK_METHOD(bool, is_connected, (), (const));
  MOCK_METHOD(void, flush_buffers, (), ());
};

class STM32F4ControllerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a controller with a mock UART device path
    controller_ = std::make_unique<hardware::STM32F4Controller>("/dev/null");
  }

  void TearDown() override {
    if (controller_) {
      controller_->cleanup();
    }
    controller_.reset();
  }

  std::unique_ptr<hardware::STM32F4Controller> controller_;
};

TEST_F(STM32F4ControllerTest, DefaultState) {
  EXPECT_FALSE(controller_->is_healthy());
  EXPECT_FALSE(controller_->is_connected());
  // get_port_id() method removed - controller now handles all ports
  // Error message may be empty initially until first operation fails
}

TEST_F(STM32F4ControllerTest, Constants) {
  EXPECT_EQ(hardware::STM32F4Controller::NUM_PORTS, 32);
  EXPECT_GT(hardware::STM32F4Controller::MAX_FREQUENCY_MHZ,
            hardware::STM32F4Controller::MIN_FREQUENCY_MHZ);
  EXPECT_GT(hardware::STM32F4Controller::MAX_SNR_DB,
            hardware::STM32F4Controller::MIN_SNR_DB);

  // Check frequency range is reasonable for L-band
  EXPECT_GE(hardware::STM32F4Controller::MIN_FREQUENCY_MHZ, 900.0);
  EXPECT_LE(hardware::STM32F4Controller::MAX_FREQUENCY_MHZ, 2200.0);
}

TEST_F(STM32F4ControllerTest, InitializationFailure) {
  // Without proper UART device, initialization should fail
  EXPECT_FALSE(controller_->initialize());
  EXPECT_FALSE(controller_->is_healthy());
  EXPECT_FALSE(controller_->is_connected());
}

TEST_F(STM32F4ControllerTest, ReadingStructure) {
  hardware::STM32F4Reading reading;

  // Test default values
  EXPECT_EQ(reading.frequency_mhz, 0.0);
  EXPECT_EQ(reading.snr_db, 0.0);
  EXPECT_EQ(reading.signal_level_dbm, -100.0);
  EXPECT_EQ(reading.raw_frequency, 0);
  EXPECT_EQ(reading.raw_snr, 0);
  EXPECT_FALSE(reading.valid);
  EXPECT_EQ(reading.port_id, 0);
}

TEST_F(STM32F4ControllerTest, PortStateControl) {
  // Test port state control methods exist and fail when not initialized
  EXPECT_FALSE(controller_->enable_port(0, true));
  EXPECT_FALSE(controller_->set_signal_detection(0, true));
}


TEST_F(STM32F4ControllerTest, ProtocolConstants) {
  using namespace hardware::protocol;

  // Verify command constants are defined
  EXPECT_NE(STM32_CMD_READ_FREQUENCY, 0);
  EXPECT_NE(STM32_CMD_READ_SNR, 0);
  EXPECT_NE(STM32_CMD_ENABLE_PORT, 0);
  EXPECT_NE(STM32_CMD_GET_STATUS, 0);
  EXPECT_NE(STM32_CMD_CALIBRATE, 0);
  EXPECT_NE(STM32_CMD_RESET, 0);
  EXPECT_NE(STM32_CMD_SIGNAL_DETECTION, 0);

  // Verify response constants
  EXPECT_EQ(STM32_RESP_OK, 0x00);
  EXPECT_EQ(STM32_RESP_ERROR, 0xFF);
  EXPECT_EQ(STM32_RESP_BUSY, 0xFE);
  EXPECT_EQ(STM32_RESP_INVALID_CMD, 0xFD);

  // Verify limits
  EXPECT_GT(STM32_MAX_PACKET_SIZE, 0);
  EXPECT_GT(STM32_TIMEOUT_MS, 0);
}

TEST_F(STM32F4ControllerTest, UninitializedOperations) {
  hardware::STM32F4Reading reading;
  std::string version;

  // All operations should fail when not initialized
  EXPECT_FALSE(controller_->read_frequency_and_snr(0, reading));
  EXPECT_FALSE(controller_->start_continuous_measurement());
  EXPECT_FALSE(controller_->enable_port(0, true));
  EXPECT_FALSE(controller_->set_signal_detection(0, true));
  EXPECT_FALSE(controller_->calibrate_frequency_detector());
  EXPECT_FALSE(controller_->reset_mcu());
  EXPECT_FALSE(controller_->get_firmware_version(version));
}

TEST_F(STM32F4ControllerTest, ContinuousMeasurementWhenNotInitialized) {
  EXPECT_FALSE(controller_->start_continuous_measurement());
  EXPECT_TRUE(
      controller_->stop_continuous_measurement()); // Should succeed even when
                                                   // not started
}

class STM32F4ManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    manager_ = std::make_unique<hardware::STM32F4Manager>();
  }

  void TearDown() override {
    if (manager_) {
      manager_->cleanup();
    }
    manager_.reset();
  }

  std::unique_ptr<hardware::STM32F4Manager> manager_;
};

TEST_F(STM32F4ManagerTest, DefaultState) {
  // Manager should not be initialized by default
  EXPECT_FALSE(manager_->are_all_ports_healthy());

  // Invalid port access should return nullptr
  EXPECT_EQ(manager_->get_controller(32), nullptr);
  EXPECT_EQ(manager_->get_controller(255), nullptr);
}

TEST_F(STM32F4ManagerTest, BulkOperations) {
  // Operations succeed when no controllers are configured (vacuous success)
  EXPECT_TRUE(manager_->calibrate_all_ports());
  EXPECT_TRUE(manager_->reset_all_mcus());
}

TEST_F(STM32F4ManagerTest, HealthMonitoring) {
  auto healthy_ports = manager_->get_healthy_ports();
  auto unhealthy_ports = manager_->get_unhealthy_ports();

  // Without initialization, all ports should be unhealthy
  EXPECT_TRUE(healthy_ports.empty());
  EXPECT_EQ(unhealthy_ports.size(), hardware::STM32F4Controller::NUM_PORTS);
  EXPECT_FALSE(manager_->are_all_ports_healthy());
}

TEST_F(STM32F4ManagerTest, ReadAllFrequencies) {
  auto readings = manager_->read_all_frequencies();

  // Should return empty vector when controllers are not initialized
  EXPECT_TRUE(readings.empty());
}

TEST_F(STM32F4ManagerTest, InvalidPortConfiguration) {
  // Should fail for invalid port IDs
  EXPECT_FALSE(manager_->configure_port(32, "/dev/null"));
  EXPECT_FALSE(manager_->configure_port(255, "/dev/null"));
}

// Test frequency and SNR conversion calculations
TEST(STM32F4Calculations, FrequencyConversion) {
  const double min_freq = hardware::STM32F4Controller::MIN_FREQUENCY_MHZ;
  const double max_freq = hardware::STM32F4Controller::MAX_FREQUENCY_MHZ;

  // Test boundary conditions
  uint16_t raw_min = 0;
  uint16_t raw_max = 65535;
  uint16_t raw_mid = 32767;

  double freq_min = min_freq + (static_cast<double>(raw_min) / 65535.0) *
                                   (max_freq - min_freq);
  double freq_max = min_freq + (static_cast<double>(raw_max) / 65535.0) *
                                   (max_freq - min_freq);
  double freq_mid = min_freq + (static_cast<double>(raw_mid) / 65535.0) *
                                   (max_freq - min_freq);

  EXPECT_DOUBLE_EQ(freq_min, min_freq);
  EXPECT_DOUBLE_EQ(freq_max, max_freq);
  EXPECT_NEAR(freq_mid, (min_freq + max_freq) / 2.0,
              1.0); // Allow 1 MHz tolerance
}

TEST(STM32F4Calculations, SNRConversion) {
  const double min_snr = hardware::STM32F4Controller::MIN_SNR_DB;
  const double max_snr = hardware::STM32F4Controller::MAX_SNR_DB;

  // Test boundary conditions
  uint16_t raw_min = 0;
  uint16_t raw_max = 65535;

  double snr_min =
      min_snr + (static_cast<double>(raw_min) / 65535.0) * (max_snr - min_snr);
  double snr_max =
      min_snr + (static_cast<double>(raw_max) / 65535.0) * (max_snr - min_snr);

  EXPECT_DOUBLE_EQ(snr_min, min_snr);
  EXPECT_DOUBLE_EQ(snr_max, max_snr);
}

TEST(STM32F4Calculations, SignalLevelConversion) {
  // Test signal level calculation
  uint16_t raw_min = 0;
  uint16_t raw_max = 65535;

  double level_min = -100.0 + (static_cast<double>(raw_min) / 65535.0) * 60.0;
  double level_max = -100.0 + (static_cast<double>(raw_max) / 65535.0) * 60.0;

  EXPECT_DOUBLE_EQ(level_min, -100.0);
  EXPECT_DOUBLE_EQ(level_max, -40.0);
}

} // namespace splitter::testing