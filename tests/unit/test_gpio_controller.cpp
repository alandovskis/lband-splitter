#include "../../src/hardware/gpio_controller.h"
#include <gtest/gtest.h>

namespace splitter::testing {

class GpioControllerTest : public ::testing::Test {
protected:
  void SetUp() override {
    controller_ = std::make_unique<hardware::GpioController>();
  }

  void TearDown() override {
    if (controller_) {
      controller_->cleanup();
    }
    controller_.reset();
  }

  std::unique_ptr<hardware::GpioController> controller_;
};

TEST_F(GpioControllerTest, DefaultState) {
  EXPECT_FALSE(controller_->is_pin_configured(100));
  EXPECT_EQ(controller_->get_pin_state(100), hardware::PinState::LOW);
  EXPECT_FALSE(controller_->is_pin_healthy(999));
}

TEST_F(GpioControllerTest, PinStateEnum) {
  EXPECT_NE(hardware::PinState::LOW, hardware::PinState::HIGH);
}

} // namespace splitter::testing