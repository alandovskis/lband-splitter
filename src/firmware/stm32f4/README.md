# STM32F4 Frequency Detector Firmware

This directory contains the firmware for the STM32F4 microcontroller that implements L-band frequency detection for the splitter system.

## Features

- **Real-time L-band frequency detection (950-2150 MHz)**
- **ADC-based signal sampling with 12-bit resolution**
- **FFT-based frequency analysis with 256-point FFT**
- **UART communication protocol compatible with host controller**
- **SNR calculation and signal quality assessment**
- **Hardware Abstraction Layers**:
  - **Display Abstraction**: Multi-type display support (SSD1306 OLED, HD44780 LCD, 7-segment)
  - **LED Abstraction**: Advanced pattern generation with GPIO and PWM support
- **Port Management**: Integrated port control using hardware abstractions
- **I2C Multiplexer Support**: TCA9548A multiplexer abstraction for display addressing
- **Autonomous LED Control**: Pattern-based LED management (blink, pulse, flash)
- **Automatic frequency calibration support**
- **Low-power operation with sleep modes**

## Hardware Requirements

- **STM32F407VG or compatible MCU**
- **8 MHz external crystal (HSE)**
- **RF frontend for L-band signal conditioning**
- **ADC input connected to RF detector**
- **UART interface for host communication**
- **Hardware Abstraction Support**:
  - **Display Hardware**: I2C displays (SSD1306 OLED recommended), I2C multiplexers (TCA9548A)
  - **LED Hardware**: GPIO pins for LED control, optional PWM timers for brightness control
  - **Port Integration**: Each port supports display + LED pair via abstractions
- **GPIO Requirements**: 64+ pins for LED control (2 per port × 32 ports)
- **I2C Master**: For display and multiplexer communication
- **Timer Peripherals**: For LED pattern timing and ADC sampling

## Compilation

### Prerequisites

1. **ARM GNU Toolchain**: Install `gcc-arm-none-eabi`
2. **STM32CubeF4**: Download and install STM32CubeF4 library
3. **CMake**: Version 3.16 or later

### Ubuntu/Debian
```bash
sudo apt install gcc-arm-none-eabi cmake
```

### macOS
```bash
brew install armmbed/formulae/gcc-arm-none-eabi cmake
```

### STM32CubeF4 Installation

Download STM32CubeF4 from ST's website and extract to `/opt/STM32CubeF4/` or set the `STM32_HAL_PATH` environment variable:

```bash
export STM32_HAL_PATH=/path/to/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver
export CMSIS_PATH=/path/to/STM32CubeF4/Drivers/CMSIS
```

### Build Process

```bash
# Create build directory
mkdir build && cd build

# Configure for Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Or configure for Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the firmware
make

# Generate additional information
make info
```

### Build Outputs

- `stm32f4_frequency_detector.elf` - ELF executable with debug symbols
- `stm32f4_frequency_detector.hex` - Intel HEX format for flashing
- `stm32f4_frequency_detector.bin` - Raw binary format
- `stm32f4_frequency_detector.list` - Disassembly listing
- `stm32f4_frequency_detector.map` - Memory map

## Flashing

### Using ST-LINK Utility
```bash
make flash
```

### Using OpenOCD
```bash
make flash_openocd
```

### Manual Flashing
```bash
# Using st-flash
st-flash write stm32f4_frequency_detector.bin 0x8000000

# Using OpenOCD
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
        -c "program stm32f4_frequency_detector.hex verify reset exit"
```

## Debugging

### GDB with OpenOCD
```bash
# Terminal 1: Start OpenOCD
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb stm32f4_frequency_detector.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

## Hardware Abstraction Architecture

### Display Abstraction (`display_abstraction.h/c`)
- **Multi-display Support**: SSD1306 OLED, HD44780 LCD, 7-segment displays
- **I2C Multiplexer Integration**: TCA9548A multiplexer support for 32+ displays
- **Content Management**: Text formatting, frequency display, two-line output
- **Hardware-Agnostic Interface**: Easy to add new display types
- **Power Management**: Enable/disable and ready status checking

### LED Abstraction (`led_abstraction.h/c`)
- **Advanced Patterns**: On/Off, Slow/Fast blink, Pulse breathing, Flash sequences
- **Multiple LED Types**: Status, Signal, Error, Activity LEDs per port
- **RGB LED Support**: Multi-color LED control with GPIO or PWM
- **Port LED Groups**: High-level port-specific LED management
- **Custom Timing**: Configurable blink rates and pattern parameters
- **Non-blocking Updates**: Pattern generation via regular `led_update()` calls

### Port Integration (`port.h/c`)
- **Hardware Abstraction Integration**: Each port uses display and LED abstractions
- **State-driven Updates**: Automatic display/LED updates based on port state
- **Clean Initialization**: `port_init_hardware()` sets up all abstractions
- **Resource Management**: Proper cleanup with `port_cleanup_hardware()`

## Protocol Interface

The firmware implements the same UART protocol as defined in the host `stm32f4_controller.h`:

### Commands
- `0x01` - Read frequency measurement
- `0x02` - Read SNR measurement  
- `0x06` - Perform calibration
- `0x07` - Reset detector
- **New**: Port control commands for display/LED management via abstractions

### Response Format
- Status byte (0x00=OK, 0xFF=Error)
- Data length
- Data payload (frequency, SNR, timestamp, display/LED status)

## Configuration

### Frequency Range
- Default: 950-2150 MHz (L-band)
- Configurable via `FREQ_MIN_MHZ` and `FREQ_MAX_MHZ` defines

### ADC Settings
- 12-bit resolution (4096 levels)
- Configurable sampling rate via timer
- Single-ended input on PA0

### FFT Parameters
- 256-point FFT for frequency analysis
- Hann windowing for improved spectral resolution
- Peak detection with harmonic rejection

## Power Consumption

- **Active measurement**: ~50mA @ 3.3V
- **Idle (sleep mode)**: ~5mA @ 3.3V
- **Deep sleep**: ~100μA @ 3.3V (RTC only)

## Memory Usage

- **Flash**: ~32KB (including HAL drivers)
- **RAM**: ~8KB (buffers and variables)
- **Stack**: ~2KB recommended minimum

## Performance

- **Measurement rate**: Up to 10 Hz continuous
- **Frequency resolution**: ~1 kHz
- **SNR range**: -30 to +40 dB
- **Signal level**: -100 to -40 dBm

## Using Hardware Abstractions

### Display Abstraction Example
```c
// Initialize display system
display_abstraction_init();

// Create display with I2C multiplexer
DisplayAbstraction* display = display_create_multiplexed(
    0,                          // port_id
    DISPLAY_TYPE_OLED_SSD1306,  // display type
    0x3C,                       // display I2C address
    0x70,                       // multiplexer address
    0,                          // multiplexer channel
    &hi2c1                      // I2C handle
);

if (display) {
    display_initialize(display);
    display_show_frequency(display, 1575.42);  // Show GPS L1
    display_show_two_lines(display, "Port 1", "1575.42 MHz");
}
```

### LED Abstraction Example
```c
// Initialize LED system
led_abstraction_init();

// Create status LED
LedAbstraction* status_led = led_create_simple(
    0,              // led_id
    0,              // port_id
    LED_TYPE_STATUS,// LED type
    GPIOA,          // GPIO port
    GPIO_PIN_0,     // GPIO pin
    true            // active high
);

if (status_led) {
    led_initialize(status_led);
    led_set_state(status_led, LED_STATE_BLINK_SLOW);
    
    // In main loop:
    led_update(status_led);  // Updates patterns
}
```

### Port Integration Example
```c
// Initialize port with abstractions
Port port;
port_init(&port, 0);
port_init_hardware(&port, &hi2c1);

// Enable port (automatically updates display and LEDs)
port_set_enabled(&port, true);
port_set_signal_detection(&port, true);
port_update_measurements(&port, 1575.42, 45.2);

// Regular updates
port_update(&port);  // Updates display content and LED patterns
```

### See Also
- `abstraction_example.c` - Complete usage examples
- `display_abstraction.h` - Display API documentation  
- `led_abstraction.h` - LED API documentation
- `port.h` - Port integration interface

## Troubleshooting

### Build Issues
- Ensure ARM toolchain is in PATH
- Verify STM32CubeF4 installation path
- Check CMake version (3.16+ required)

### Runtime Issues
- Verify crystal oscillator (8 MHz HSE)
- Check UART connections (115200 baud)
- Confirm ADC input signal levels
- Review power supply stability (3.3V ±5%)

### Hardware Abstraction Issues
- **Display Problems**: Check I2C connections, verify multiplexer addressing
- **LED Issues**: Confirm GPIO pin assignments, check active high/low configuration
- **Pattern Problems**: Ensure `led_update()` called regularly in main loop
- **I2C Multiplexer**: Verify TCA9548A addressing and channel selection

### Communication Problems
- Test UART loopback
- Verify baud rate settings
- Check protocol packet format
- Monitor for checksum errors