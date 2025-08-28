#pragma once

#include "../../src/hardware/frequency_detector.h"
#include "../../src/hardware/led_controller.h"
#include "../../src/hardware/stm32f4_controller.h"
#include <gmock/gmock.h>

namespace splitter::testing {

class MockFrequencyDetector {
public:
  MOCK_METHOD(bool, initialize, ());
  MOCK_METHOD(void, cleanup, ());

  MOCK_METHOD(double, measure_frequency, (int port_id));
  MOCK_METHOD(double, get_frequency, (int port_id), (const));
  MOCK_METHOD(double, get_signal_level, (int port_id), (const));

  MOCK_METHOD(hardware::FrequencyReading, get_reading, (int port_id), (const));
  MOCK_METHOD(std::vector<hardware::FrequencyReading>, get_all_readings, (),
              (const));

  MOCK_METHOD(void, start_continuous_measurement, ());
  MOCK_METHOD(void, stop_continuous_measurement, ());

  MOCK_METHOD(bool, is_healthy, (), (const));
  MOCK_METHOD(std::string, get_last_error, (), (const));
};

class MockLedController {
public:
  MOCK_METHOD(bool, initialize, ());
  MOCK_METHOD(void, cleanup, ());

  MOCK_METHOD(bool, configure_led, (int led_id));
  MOCK_METHOD(bool, configure_led_with_pin, (int led_id, int gpio_pin));

  MOCK_METHOD(bool, set_led_state, (int led_id, hardware::LedState state));
  MOCK_METHOD(bool, set_led_on, (int led_id));
  MOCK_METHOD(bool, set_led_off, (int led_id));
  MOCK_METHOD(bool, set_led_blinking_slow, (int led_id));
  MOCK_METHOD(bool, set_led_blinking_fast, (int led_id));

  MOCK_METHOD(hardware::LedState, get_led_state, (int led_id), (const));
  MOCK_METHOD(bool, is_led_configured, (int led_id), (const));
  MOCK_METHOD(bool, is_led_healthy, (int led_id), (const));

  MOCK_METHOD(void, start_blink_thread, ());
  MOCK_METHOD(void, stop_blink_thread, ());

  MOCK_METHOD(bool, is_healthy, (), (const));
  MOCK_METHOD(std::string, get_last_error, (), (const));
};

class MockSTM32F4Controller {
public:
  MOCK_METHOD(bool, initialize, ());
  MOCK_METHOD(void, cleanup, ());

  MOCK_METHOD(bool, read_frequency_and_snr,
              (hardware::STM32F4Reading & reading));
  MOCK_METHOD(hardware::STM32F4Reading, get_last_reading, (), (const));
  MOCK_METHOD(bool, start_continuous_measurement, ());
  MOCK_METHOD(bool, stop_continuous_measurement, ());

  MOCK_METHOD(bool, set_led_state, (const hardware::STM32F4LedState &state));
  MOCK_METHOD(bool, set_status_led, (bool on, uint8_t brightness = 255));
  MOCK_METHOD(bool, set_signal_led, (bool on, uint8_t brightness = 255));
  MOCK_METHOD(bool, set_led_blinking,
              (bool status_led, bool signal_led, uint16_t period_ms = 1000));

  MOCK_METHOD(bool, update_display, (const hardware::STM32F4DisplayData &data));
  MOCK_METHOD(bool, set_display_brightness, (uint8_t brightness));
  MOCK_METHOD(bool, clear_display, ());

  MOCK_METHOD(bool, calibrate_frequency_detector, ());
  MOCK_METHOD(bool, reset_mcu, ());
  MOCK_METHOD(bool, get_firmware_version, (std::string & version));

  MOCK_METHOD(bool, is_healthy, (), (const));
  MOCK_METHOD(bool, is_connected, (), (const));
  MOCK_METHOD(std::string, get_last_error, (), (const));
  MOCK_METHOD(uint8_t, get_port_id, (), (const));
};

} // namespace splitter::testing