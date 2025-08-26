#include "../../src/hardware/led_controller.h"
#include "../mocks/mock_gpio_controller.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace splitter::testing {

class LedControllerTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_gpio_ = std::make_unique<MockableGpioController>();
    controller_ = std::make_unique<hardware::LedController>(mock_gpio_.get());
  }

  void TearDown() override {
    if (controller_) {
      controller_->cleanup();
    }
    controller_.reset();
    mock_gpio_.reset();
  }

  std::unique_ptr<MockableGpioController> mock_gpio_;
  std::unique_ptr<hardware::LedController> controller_;
};

TEST_F(LedControllerTest, Initialize) {
  EXPECT_TRUE(controller_->initialize());
  EXPECT_FALSE(controller_->is_led_configured(0));
}

TEST_F(LedControllerTest, ConfigureLed) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).WillOnce(Return(true));

  ASSERT_TRUE(controller_->initialize());
  EXPECT_TRUE(controller_->configure_led(0));
  EXPECT_TRUE(controller_->is_led_configured(0));
}

TEST_F(LedControllerTest, SetLedStates) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_high(_)).WillOnce(Return(true));

  ASSERT_TRUE(controller_->initialize());
  ASSERT_TRUE(controller_->configure_led(0));

  EXPECT_TRUE(controller_->set_led_on(0));
  EXPECT_EQ(controller_->get_led_state(0), hardware::LedState::ON);

  EXPECT_TRUE(controller_->set_led_off(0));
  EXPECT_EQ(controller_->get_led_state(0), hardware::LedState::OFF);
}

TEST_F(LedControllerTest, BlinkingStates) {
  EXPECT_CALL(*mock_gpio_, configure_output_pin(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_low(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock_gpio_, set_pin_high(_)).WillOnce(Return(true));

  ASSERT_TRUE(controller_->initialize());
  ASSERT_TRUE(controller_->configure_led(0));

  EXPECT_TRUE(controller_->set_led_blinking_slow(0));
  EXPECT_EQ(controller_->get_led_state(0), hardware::LedState::BLINKING_SLOW);

  EXPECT_TRUE(controller_->set_led_blinking_fast(0));
  EXPECT_EQ(controller_->get_led_state(0), hardware::LedState::BLINKING_FAST);
}

} // namespace splitter::testing