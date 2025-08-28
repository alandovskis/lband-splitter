#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../src/core/port.h"
#include "../mocks/mock_gpio_controller.h"
#include "../mocks/mock_hardware.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace splitter::testing {

class PortTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_gpio_ = std::make_unique<MockableGpioController>();
    mock_stm32f4_ = std::make_unique<MockSTM32F4Controller>();
    port_ =
        std::make_unique<core::Port>(0, mock_gpio_.get(), mock_stm32f4_.get());
  }

  void TearDown() override {
    port_.reset();
    mock_stm32f4_.reset();
    mock_gpio_.reset();
  }

  std::unique_ptr<MockableGpioController> mock_gpio_;
  std::unique_ptr<MockSTM32F4Controller> mock_stm32f4_;
  std::unique_ptr<core::Port> port_;
};

TEST_F(PortTest, InitializeSuccess) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, initialize()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, set_led_state(_)).Times(1).WillOnce(Return(true));

  EXPECT_TRUE(port_->initialize());
}

TEST_F(PortTest, InitializeFailure) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_))
      .Times(1)
      .WillOnce(Return(false));

  EXPECT_FALSE(port_->initialize());
}

TEST_F(PortTest, EnablePort) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, initialize()).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, set_led_state(_))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_high(_)).WillOnce(Return(true));

  ASSERT_TRUE(port_->initialize());
  EXPECT_TRUE(port_->enable());
  EXPECT_TRUE(port_->is_enabled());
}

TEST_F(PortTest, DisablePort) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, initialize()).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_stm32f4_, set_led_state(_))
      .Times(3)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_high(_)).WillOnce(Return(true));

  ASSERT_TRUE(port_->initialize());
  ASSERT_TRUE(port_->enable());
  ASSERT_TRUE(port_->is_enabled());

  EXPECT_TRUE(port_->disable());
  EXPECT_FALSE(port_->is_enabled());
}

TEST_F(PortTest, GetPortState) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, initialize()).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, set_led_state(_)).WillOnce(Return(true));

  ASSERT_TRUE(port_->initialize());

  auto state = port_->get_state();
  EXPECT_EQ(state.id, 0);
  EXPECT_FALSE(state.enabled);
  EXPECT_TRUE(state.healthy);
}

TEST_F(PortTest, PortConfiguration) {
  core::PortConfig config;
  config.name = "Test Port";
  config.auto_enable = true;
  config.min_frequency_mhz = 1200.0;
  config.max_frequency_mhz = 1800.0;
  config.gain_db = 10;
  config.signal_detection_enabled = true;

  EXPECT_TRUE(port_->set_configuration(config));

  auto retrieved_config = port_->get_configuration();
  EXPECT_EQ(retrieved_config.name, "Test Port");
  EXPECT_TRUE(retrieved_config.auto_enable);
  EXPECT_DOUBLE_EQ(retrieved_config.min_frequency_mhz, 1200.0);
  EXPECT_DOUBLE_EQ(retrieved_config.max_frequency_mhz, 1800.0);
  EXPECT_EQ(retrieved_config.gain_db, 10);
  EXPECT_TRUE(retrieved_config.signal_detection_enabled);
}

TEST_F(PortTest, InvalidConfiguration) {
  core::PortConfig invalid_config;
  invalid_config.min_frequency_mhz = 2000.0;
  invalid_config.max_frequency_mhz = 1000.0;

  EXPECT_FALSE(port_->set_configuration(invalid_config));
}

TEST_F(PortTest, FrequencyUpdate) {
  double test_frequency = 1500.0;
  port_->update_frequency(test_frequency);

  auto state = port_->get_state();
  EXPECT_DOUBLE_EQ(state.frequency_mhz, test_frequency);
}

TEST_F(PortTest, SignalLevelUpdate) {
  double test_level = -45.5;
  port_->update_signal_level(test_level);

  auto state = port_->get_state();
  EXPECT_DOUBLE_EQ(state.signal_level_dbm, test_level);
}

TEST_F(PortTest, HealthCheck) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, initialize()).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, set_led_state(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, is_pin_healthy(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_stm32f4_, is_healthy()).WillOnce(Return(true));

  ASSERT_TRUE(port_->initialize());

  port_->check_health();
  auto state = port_->get_state();
  EXPECT_TRUE(state.healthy);
}

} // namespace splitter::testing