# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a 32-port L-band splitter/combiner system running on Linux, written in C++ with an Angular web interface. The system provides hardware control for RF signal splitting/combining with visual feedback and remote management capabilities.

## Architecture

### Core Components
- **C++ Backend**: Hardware control daemon for 32 L-band ports
- **Angular Frontend**: Web-based control interface
- **NetConf Interface**: Standards-based network configuration protocol
- **Hardware Abstraction Layer**: GPIO control for LEDs and frequency detection

### Port Features (per port × 32)
- Enable/disable control with status LED
- L-band signal detection with indicator LED
- Center frequency detection and display on small screen
- Individual port configuration via web interface and NetConf

## Development Commands

### C++ Backend
```bash
# Install Conan dependencies
conan install . --build=missing

# Build the C++ daemon (Release)
cmake --preset conan-release
cmake --build --preset conan-release

# Build with debug symbols
conan install . --build=missing --settings=build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug

# Run unit tests
cmake --build --preset conan-release
ctest --test-dir build/Release --verbose

# Format C++ files
find src tests -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format -i

# Run static analysis before committing
cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem --suppress=syntaxError:src/netconf/yang_model.cpp src/

# Install system service
cmake --install build/Release --prefix /usr/local
sudo make install_service
```

### Angular Frontend
```bash
# Install dependencies
npm install

# Development server
npm run dev

# Build for production
npm run build

# Run tests
npm test

# Lint code
npm run lint
```

### System Integration
```bash
# Start the splitter daemon
sudo systemctl start splitter

# View logs
journalctl -u splitter -f

# NetConf server (if separate)
sudo systemctl start netconf-server
```

## Key Directories

- `src/`: C++ source code
  - `hardware/`: Hardware abstraction layer (GPIO, SPI, I2C)
  - `netconf/`: NetConf protocol implementation
  - `web/`: REST API server for Angular frontend
  - `core/`: Main splitter logic and port management
- `webapp/`: Angular application
  - `src/app/components/`: UI components for port control
  - `src/app/services/`: API services for backend communication
- `config/`: Configuration files and schemas
- `scripts/`: Build and deployment scripts
- `tests/`: Unit and integration tests

## Hardware Integration Notes

### GPIO Requirements
- 64 GPIO pins for LEDs (2 per port × 32 ports)
- SPI/I2C buses for frequency detection circuits
- Hardware-specific drivers in `src/hardware/`

### Frequency Detection
- ADCs for L-band center frequency measurement
- Real-time processing for frequency analysis
- Display drivers for small screens per port

### RF Switching
- Control signals for L-band splitter/combiner matrices
- Power management for active components

## NetConf Implementation

The system uses libnetconf2 for standards-compliant network management:
- YANG models in `config/yang/`
- Configuration datastore management
- Notification support for hardware events

## Development Guidelines

- Follow modern C++17 standards with RAII and smart pointers
- **All C++ files must be formatted using clang-format** - run `clang-format -i` on modified files
- **Run cppcheck before every commit** - `cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem --suppress=syntaxError:src/netconf/yang_model.cpp src/`
- Use Conan for dependency management - update conanfile.txt for new dependencies
- Use Angular Material for consistent UI components
- Implement proper error handling for hardware failures
- Log all hardware state changes for debugging
- Use systemd for service management
- Implement graceful degradation when ports fail
- Use CMake presets for consistent builds (conan-release, conan-debug)

## Testing Strategy

### Hardware Testing
- Mock hardware interfaces for unit testing
- Integration tests with actual GPIO hardware
- RF signal path verification tests

### Software Testing
- Unit tests for all C++ classes
- Angular component testing with Jasmine/Karma
- End-to-end testing of web interface
- NetConf protocol compliance testing

## Configuration Management

- YAML configuration for port assignments
- Runtime configuration via NetConf
- Persistent storage of port states
- Configuration validation and rollback support