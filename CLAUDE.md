# RF Splitter Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-09-06

## Active Technologies
- C++ 17/20 + Angular 17+ + STM32 bare-metal C (001-you-are-developing)

## Project Structure
```
backend/src/          # C++ backend services
frontend/src/         # Angular web interface  
firmware/src/         # STM32F4 embedded firmware
tests/               # Test suites
```

## Commands
# Backend C++ commands
make build           # Build C++ backend
make test           # Run gtest unit tests  
make integration    # Run hardware integration tests

# Frontend Angular commands  
npm run build       # Build Angular app
npm run test        # Run Jasmine/Karma tests
npm run e2e         # End-to-end tests

# Firmware commands
make firmware       # Build STM32 firmware
make flash          # Flash to STM32F4
make test-hardware  # Hardware-in-loop tests

## Code Style
C++: Follow Google C++ Style Guide, use STL containers
Angular: Follow Angular Style Guide, use OnPush change detection
STM32: Use STM32 HAL, CMSIS-DSP for signal processing

## Key Libraries  
- libwebsockets: Real-time C++ WebSocket communication
- CMSIS-DSP: ARM DSP library for FFT analysis on STM32F4
- Angular Material: UI components for RF dashboard
- libnetconf2: Network configuration management

## RF-Specific Notes
- L-Band frequency range: 950-2450 MHz  
- Signal integrity requirements per FR-014
- External RF front-end required for down-conversion
- Real-time processing: 60Hz measurement updates

## Recent Changes
- 001-you-are-developing: Added L-Band RF splitter/combiner system with C++ backend, Angular frontend, STM32 firmware

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->