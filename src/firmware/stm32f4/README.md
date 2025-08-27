# STM32F4 Frequency Detector Firmware

This directory contains the firmware for the STM32F4 microcontroller that implements L-band frequency detection for the splitter system.

## Features

- **Real-time L-band frequency detection (950-2150 MHz)**
- **ADC-based signal sampling with 12-bit resolution**
- **FFT-based frequency analysis with 256-point FFT**
- **UART communication protocol compatible with host controller**
- **SNR calculation and signal quality assessment**
- **Automatic frequency calibration support**
- **Low-power operation with sleep modes**

## Hardware Requirements

- **STM32F407VG or compatible MCU**
- **8 MHz external crystal (HSE)**
- **RF frontend for L-band signal conditioning**
- **ADC input connected to RF detector**
- **UART interface for host communication**
- **Status LEDs and optional display**

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

## Protocol Interface

The firmware implements the same UART protocol as defined in the host `stm32f4_controller.h`:

### Commands
- `0x01` - Read frequency measurement
- `0x02` - Read SNR measurement  
- `0x06` - Perform calibration
- `0x07` - Reset detector

### Response Format
- Status byte (0x00=OK, 0xFF=Error)
- Data length
- Data payload (frequency, SNR, timestamp)

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

### Communication Problems
- Test UART loopback
- Verify baud rate settings
- Check protocol packet format
- Monitor for checksum errors