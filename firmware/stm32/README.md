# STM32 Firmware for L-Band Splitter

This directory contains the STM32 firmware for the 32-port L-Band splitter/combiner hardware control.

## Overview

The STM32 microcontroller serves as the hardware interface layer, managing:
- 64 GPIO pins for LED control (32 enable + 32 signal LEDs)
- 32 ADC channels for frequency detection
- 32 small OLED displays for frequency readout
- I2C communication with the main Linux system
- Real-time hardware monitoring and control

## Hardware Configuration

### STM32 Model
- **Recommended**: STM32F407VET6 or STM32F429ZIT6
- **Requirements**: 
  - Minimum 64 GPIO pins
  - 32+ ADC channels (or multiplexed)
  - I2C, SPI, UART interfaces
  - 512KB+ Flash, 128KB+ RAM

### Pin Assignments

```
GPIO Pins:
- PA0-PA15: Port 0-15 Enable LEDs
- PB0-PB15: Port 16-31 Enable LEDs  
- PC0-PC15: Port 0-15 Signal LEDs
- PD0-PD15: Port 16-31 Signal LEDs

ADC Channels:
- ADC1: Channels 0-15 (Ports 0-15 frequency detection)
- ADC2: Channels 0-15 (Ports 16-31 frequency detection)

Communication:
- I2C1: Communication with Linux host (SCL: PB8, SDA: PB9)
- SPI1: Display controller interface (SCK: PA5, MISO: PA6, MOSI: PA7)
- UART1: Debug/backup communication (TX: PA9, RX: PA10)

Other:
- PA8: System status LED
- PB0: Hardware reset input
```

## Firmware Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Communication   │◄──►│   Main Control  │◄──►│  Hardware HAL   │
│   Protocol      │    │     Loop        │    │    Drivers      │
│ (I2C/SPI/UART)  │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         ▲                       ▲                       ▲
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Command Parser  │    │ Port Management │    │ ADC Sampling    │
│ & Response Gen  │    │   & Control     │    │ & Processing    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Command Protocol

The firmware implements a packet-based protocol:

```c
struct command_packet {
    uint8_t header;     // 0xAA
    uint8_t command;    // Command code
    uint8_t length;     // Data length
    uint8_t data[];     // Command data
    uint8_t checksum;   // XOR checksum
};

struct response_packet {
    uint8_t header;     // 0x55
    uint8_t status;     // Status code
    uint8_t length;     // Data length
    uint8_t data[];     // Response data
    uint8_t checksum;   // XOR checksum
};
```

### Command Set

| Command | Code | Description | Data Format |
|---------|------|-------------|-------------|
| ENABLE_PORT | 0x01 | Enable port | port_number (1 byte) |
| DISABLE_PORT | 0x02 | Disable port | port_number (1 byte) |
| GET_PORT_STATUS | 0x03 | Get port status | port_number (1 byte) |
| SET_ENABLE_LED | 0x10 | Control enable LED | port_number, state (2 bytes) |
| SET_SIGNAL_LED | 0x11 | Control signal LED | port_number, state (2 bytes) |
| GET_FREQUENCY | 0x20 | Get frequency | port_number (1 byte) |
| GET_POWER_LEVEL | 0x21 | Get power level | port_number (1 byte) |
| UPDATE_DISPLAY | 0x30 | Update display | port_number, text[16] (17 bytes) |
| GET_SYSTEM_INFO | 0x40 | Get system info | none |
| RUN_SELF_TEST | 0x42 | Run self test | none |
| CALIBRATE_ADC | 0x43 | Calibrate ADCs | none |

## Development Setup

### Prerequisites

1. **STM32CubeIDE** or **STM32CubeMX** + **ARM GCC Toolchain**
2. **ST-Link Debugger** or compatible programmer
3. **HAL Libraries** for your specific STM32 model

### Project Structure

```
firmware/stm32/
├── Core/
│   ├── Inc/           # Header files
│   │   ├── main.h
│   │   ├── stm32f4xx_hal_conf.h
│   │   └── ...
│   └── Src/           # Source files
│       ├── main.c
│       ├── stm32f4xx_it.c
│       └── ...
├── Drivers/           # HAL drivers
├── Middlewares/       # Optional middleware
├── Application/       # Application-specific code
│   ├── port_control.c
│   ├── adc_manager.c
│   ├── display_driver.c
│   ├── comm_protocol.c
│   └── ...
└── Makefile          # Build configuration
```

### Building

1. **Using STM32CubeIDE:**
   ```bash
   # Import project into STM32CubeIDE
   # Build with Ctrl+B or Project -> Build Project
   ```

2. **Using Command Line:**
   ```bash
   cd firmware/stm32
   make clean
   make all
   make flash  # Program the microcontroller
   ```

## Key Firmware Modules

### 1. Port Control Manager (`port_control.c`)
```c
typedef struct {
    uint8_t port_number;
    bool enabled;
    bool signal_detected;
    float frequency_mhz;
    float power_dbm;
    uint32_t last_update_ms;
} port_status_t;

void port_control_init(void);
void port_enable(uint8_t port);
void port_disable(uint8_t port);
port_status_t port_get_status(uint8_t port);
```

### 2. ADC Manager (`adc_manager.c`)
```c
void adc_manager_init(void);
void adc_start_continuous_conversion(void);
float adc_get_frequency(uint8_t port);
float adc_get_power_level(uint8_t port);
void adc_calibrate(void);
```

### 3. Communication Protocol (`comm_protocol.c`)
```c
void comm_protocol_init(void);
void comm_protocol_process(void);
void comm_send_response(uint8_t status, uint8_t* data, uint8_t length);
```

### 4. Display Driver (`display_driver.c`)
```c
void display_init(void);
void display_update_port(uint8_t port, const char* text);
void display_set_brightness(uint8_t port, uint8_t brightness);
```

## Configuration

### ADC Configuration
- **Resolution**: 12-bit (4096 levels)
- **Reference**: Internal 3.3V reference
- **Sampling Rate**: 1 MSPS per channel
- **Averaging**: 64 samples for noise reduction

### Timer Configuration
- **TIM2**: 1ms system tick for timing
- **TIM3**: ADC trigger timer (100Hz per channel)
- **TIM4**: Display refresh timer (10Hz)

### Interrupt Priorities
```c
// Highest priority
#define I2C_IRQ_PRIORITY        0
#define ADC_DMA_IRQ_PRIORITY    1
#define SYSTICK_IRQ_PRIORITY    2
#define GPIO_IRQ_PRIORITY       3
// Lowest priority
```

## Testing and Validation

### Unit Tests
```bash
# Run embedded unit tests
make test

# Test with hardware-in-the-loop
make test-hil
```

### Debug Interface
The firmware provides debug output via UART1:
```
[DEBUG] Port 5 enabled, ADC reading: 2048
[INFO] I2C command received: 0x03
[ERROR] ADC calibration failed on channel 12
```

### Performance Monitoring
- **CPU Usage**: < 50% at full load
- **Memory Usage**: < 80% of available RAM
- **Response Time**: < 10ms for all commands
- **ADC Update Rate**: 100Hz per port

## Hardware Integration

### Linux Host Communication
The STM32 communicates with the Linux host system via I2C at address 0x42:

```bash
# Linux side testing
i2cget -y 1 0x42 0x40  # Get system info
i2cset -y 1 0x42 0x01 5  # Enable port 5
```

### Frequency Detection Circuit
Each port connects to a frequency detection circuit:
```
RF Input → Frequency Discriminator → ADC Input
          ↓
    Power Detector → ADC Input
```

### Display Interface
32 small OLED displays (128x32) connected via SPI daisy chain:
```
STM32 SPI → Display Controller → OLED 0 → OLED 1 → ... → OLED 31
```

## Production Deployment

### Bootloader
- **Custom bootloader** for field updates via I2C
- **Firmware verification** with CRC32 checksum
- **Rollback capability** if update fails

### Manufacturing Test
```c
// Built-in manufacturing test sequence
bool manufacturing_test(void) {
    if (!test_gpio_pins()) return false;
    if (!test_adc_channels()) return false;
    if (!test_displays()) return false;
    if (!test_communication()) return false;
    return true;
}
```

### Quality Assurance
- **Static code analysis** with PC-lint
- **Runtime checks** with stack monitoring
- **Watchdog timer** for system recovery
- **Brown-out protection** for power failures

## Troubleshooting

### Common Issues

1. **I2C Communication Fails**
   - Check pull-up resistors (4.7kΩ recommended)
   - Verify clock speed (100kHz standard)
   - Check address conflicts

2. **ADC Readings Unstable**
   - Verify reference voltage stability
   - Check analog ground connections
   - Increase averaging samples

3. **Display Updates Slow**
   - Optimize SPI clock speed
   - Use DMA for display data transfer
   - Reduce display refresh rate if needed

### Debug Tools

```bash
# Connect ST-Link debugger
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# GDB debugging session
arm-none-eabi-gdb firmware.elf
(gdb) target extended-remote localhost:3333
(gdb) load
(gdb) continue
```

## Contributing

1. Follow STM32 HAL coding standards
2. Add unit tests for new features
3. Update this documentation
4. Test with actual hardware before submitting

## License

Same as main project license.