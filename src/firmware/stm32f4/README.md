# STM32F4 Frequency Detector Firmware

This directory contains the firmware for the STM32F4 microcontroller that implements L-band frequency detection for the splitter system.

## Features

- **Real-time L-band frequency detection (950-2150 MHz)**
- **ADC-based signal sampling with 12-bit resolution**
- **FFT-based frequency analysis with 256-point FFT**
- **UART communication protocol compatible with host controller**
- **SNR calculation and signal quality assessment**
- **Secure Boot System**:
  - **A/B Firmware Banks**: Dual ~496KB firmware banks for atomic updates
  - **Ed25519 Signature Verification**: Cryptographic firmware authentication
  - **Rollback Protection**: Anti-rollback counters prevent firmware downgrades
  - **Automatic Fallback**: Intelligent bank selection with failure tracking
  - **Hardware Security**: Flash protection, JTAG disable, Memory Protection Unit
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

## Secure Boot System

The STM32F4 firmware includes a comprehensive secure boot system with A/B firmware banks, cryptographic verification, and rollback protection.

### Architecture Overview

**Memory Layout:**
- **Bootloader**: 0x08000000 - 0x08008000 (32KB)
- **Firmware Bank A**: 0x08008000 - 0x0807C000 (~496KB) 
- **Firmware Bank B**: 0x0807C000 - 0x080F0000 (~496KB)
- **Boot Configuration**: Stored in dedicated flash sector with CRC protection

**Key Features:**
- **Ed25519 Signature Verification**: Cryptographic firmware authentication
- **SHA-256 Integrity Checking**: Firmware hash validation
- **CRC32 Protection**: Header and payload integrity verification
- **Rollback Protection**: Anti-rollback counters prevent firmware downgrades
- **A/B Bank System**: Atomic firmware updates with automatic fallback
- **Failure Tracking**: Intelligent boot selection with failure counters
- **Hardware Security**: Flash protection, JTAG disable, MPU configuration

### Secure Boot Build Targets

The CMake build system provides comprehensive targets for secure boot development:

#### Setup and Key Management
```bash
# Generate Ed25519 signing keypair (development)
make generate_keys

# Setup complete development environment  
make dev_setup

# Configure signing keys for builds
cmake -DSIGNING_KEY_PATH=/path/to/private_key.pem -DPUBLIC_KEY_PATH=/path/to/public_key.pem .
```

#### Building Firmware Components
```bash
# Build main application firmware
make stm32f4_frequency_detector

# Build secure bootloader
make stm32f4_frequency_detector_bootloader

# Sign firmware with secure headers
make sign_firmware

# Verify signed firmware before deployment
make verify_firmware
```

#### Flashing Operations
```bash
# Flash bootloader to beginning of flash (0x8000000)
make flash_bootloader

# Flash signed firmware to Bank A (0x8008000)
make flash_signed_a

# Flash signed firmware to Bank B (0x807C000) 
make flash_signed_b

# Complete system deployment (bootloader + firmware)
make flash_complete_system
```

#### Development and Debug
```bash
# Show signed firmware information
make firmware_info

# Display all available secure boot targets
make help_secure_boot
```

### Configuration Options

**CMake Variables:**
- `FIRMWARE_VERSION`: Version number for rollback protection (default: 1)
- `ROLLBACK_COUNTER`: Anti-rollback counter value (default: 1)
- `SIGNING_KEY_PATH`: Path to Ed25519 private key for signing
- `PUBLIC_KEY_PATH`: Path to Ed25519 public key for verification

**Build-time Security Settings:**
- `SECURE_BOOT_ENABLE_DEBUG`: Enable debug output (default: 1 for development)
- `SECURE_BOOT_ENABLE_ROLLBACK_PROTECTION`: Enable version checking (default: 1)
- `SECURE_BOOT_ENABLE_DEVELOPMENT_MODE`: Development features (default: 0)

### Production Deployment Workflow

1. **Initial Setup** (Once)
   ```bash
   # Generate production signing keys (keep private key secure!)
   ../../scripts/firmware_sign.py keygen --private-key production_key.pem --public-key production_pub.pem
   
   # Configure build system
   cmake -DCMAKE_BUILD_TYPE=Release -DSIGNING_KEY_PATH=production_key.pem .
   ```

2. **Build and Deploy Bootloader** (Initial deployment)
   ```bash
   # Build and flash secure bootloader
   make stm32f4_frequency_detector_bootloader
   make flash_bootloader
   ```

3. **Deploy Initial Firmware** (Bank A)
   ```bash
   # Build, sign, and verify firmware
   make stm32f4_frequency_detector
   make sign_firmware
   make verify_firmware
   
   # Flash to Bank A
   make flash_signed_a
   ```

4. **Over-the-Air Updates** (Production)
   ```bash
   # Update firmware version
   cmake -DFIRMWARE_VERSION=2 -DROLLBACK_COUNTER=2 .
   
   # Build and sign new firmware
   make sign_firmware
   make verify_firmware
   
   # Deploy to inactive bank (B)
   make flash_signed_b
   
   # System automatically switches to Bank B on next boot
   # Falls back to Bank A if Bank B fails to boot
   ```

### A/B Firmware Update Process

**Atomic Updates:**
1. **Preparation**: New firmware written to inactive bank
2. **Verification**: Bootloader verifies signature and integrity
3. **Activation**: Boot configuration updated atomically
4. **Fallback**: Automatic rollback on boot failures (3 attempts)

**Bank Selection Logic:**
1. Check for pending update (use pending bank)
2. Try active bank if failure count < 3
3. Try fallback bank if primary bank failed
4. Enter recovery mode if both banks failed

### Security Features

**Cryptographic Protection:**
- **Ed25519 Signatures**: 256-bit elliptic curve signatures for firmware authentication
- **SHA-256 Hashing**: 256-bit cryptographic hash for integrity verification  
- **CRC32 Checksums**: Fast integrity checking for headers and configuration

**Hardware Security:**
- **Flash Write Protection**: Prevent unauthorized firmware modification
- **JTAG Debug Disable**: Block debug access in production builds
- **Memory Protection Unit**: Isolate bootloader and application memory regions
- **Secure Boot Configuration**: Protected boot parameters with CRC validation

**Anti-Tampering:**
- **Rollback Protection**: Prevent firmware downgrade attacks
- **Boot Failure Tracking**: Detect and respond to repeated boot failures
- **Configuration Integrity**: CRC-protected boot configuration storage
- **Emergency Recovery**: Safe mode for firmware recovery operations

### LED Status Indicators

The bootloader provides LED status indicators for different boot states:

- **3 fast blinks**: Secure boot initialization error
- **5 very fast blinks**: Recovery mode active
- **8 medium blinks**: Bank selection failed
- **9 blinks**: Invalid bank address
- **10 slow blinks**: Firmware verification failed
- **20 rapid blinks**: Jump to application failed
- **Solid on**: Firmware verification in progress
- **Off**: Normal operation, jumping to application

### Troubleshooting Secure Boot

**Common Issues:**
- **Signature Verification Failed**: Check public/private key pair matches
- **Invalid Firmware Header**: Ensure firmware was properly signed
- **Bank Selection Failed**: Verify firmware banks contain valid signed firmware
- **Rollback Protection**: Increase version/rollback counter for updates

**Recovery Procedures:**
- **Button Recovery**: Hold PC13 button during boot to enter recovery mode
- **Emergency Recovery**: Bootloader enters recovery after 3 consecutive failures
- **Manual Recovery**: Flash new signed firmware to working bank
- **Complete Recovery**: Re-flash bootloader and signed firmware

**Debug Information:**
```bash
# Show detailed firmware information
make firmware_info

# Verify firmware without flashing
../../scripts/firmware_verify.py verify --firmware signed_firmware.bin --public-key public_key.pem

# Extract firmware for analysis  
../../scripts/firmware_verify.py extract --signed-firmware signed_firmware.bin --output raw_firmware.bin
```

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