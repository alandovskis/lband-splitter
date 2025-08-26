#pragma once

#include <gmock/gmock.h>
#include "../../src/hardware/gpio_controller.h"

namespace splitter::testing {

class MockGpioController {
public:
    MOCK_METHOD(bool, initialize, ());
    MOCK_METHOD(void, cleanup, ());
    
    MOCK_METHOD(bool, configure_input_pin, (int pin));
    MOCK_METHOD(bool, configure_output_pin, (int pin));
    
    MOCK_METHOD(bool, set_pin_high, (int pin));
    MOCK_METHOD(bool, set_pin_low, (int pin));
    MOCK_METHOD(bool, set_pin_state, (int pin, hardware::PinState state));
    
    MOCK_METHOD(hardware::PinState, get_pin_state, (int pin), (const));
    MOCK_METHOD(bool, is_pin_configured, (int pin), (const));
    MOCK_METHOD(bool, is_pin_healthy, (int pin), (const));
    
    MOCK_METHOD(bool, is_healthy, (), (const));
    MOCK_METHOD(std::string, get_last_error, (), (const));
};

class MockableGpioController : public hardware::GpioController {
public:
    MockableGpioController() {
        ON_CALL(*this, initialize()).WillByDefault(::testing::Return(true));
        ON_CALL(*this, is_healthy()).WillByDefault(::testing::Return(true));
        ON_CALL(*this, configure_input_pin(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, configure_output_pin(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, set_pin_high(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, set_pin_low(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, set_pin_state(::testing::_, ::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, get_pin_state(::testing::_)).WillByDefault(::testing::Return(hardware::PinState::LOW));
        ON_CALL(*this, is_pin_configured(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, is_pin_healthy(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, get_last_error()).WillByDefault(::testing::Return(""));
    }
    
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    
    MOCK_METHOD(bool, configure_input_pin, (int pin), (override));
    MOCK_METHOD(bool, configure_output_pin, (int pin), (override));
    
    MOCK_METHOD(bool, set_pin_high, (int pin), (override));
    MOCK_METHOD(bool, set_pin_low, (int pin), (override));
    MOCK_METHOD(bool, set_pin_state, (int pin, hardware::PinState state), (override));
    
    MOCK_METHOD(hardware::PinState, get_pin_state, (int pin), (const, override));
    MOCK_METHOD(bool, is_pin_configured, (int pin), (const, override));
    MOCK_METHOD(bool, is_pin_healthy, (int pin), (const, override));
    
    MOCK_METHOD(bool, is_healthy, (), (const, override));
    MOCK_METHOD(std::string, get_last_error, (), (const, override));
};

} // namespace splitter::testing