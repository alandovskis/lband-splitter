# Research Report: L-Band RF Splitter/Combiner

## Technology Decisions

### C++ WebSocket Library
**Decision**: libwebsockets  
**Rationale**: Designed for embedded Linux systems with minimal memory footprint and ARM optimization. Single-threaded non-blocking event loop suitable for real-time RF data streaming.  
**Alternatives considered**: websocketpp (heavier C++ library), uWebSockets (performance-focused but complex)

### STM32F4 DSP Capabilities 
**Decision**: STM32F4 with external RF down-conversion  
**Rationale**: STM32F4 cannot directly sample L-Band (950-2450MHz). Solution: External RF front-end down-converts L-Band to baseband, then STM32F4 processes using CMSIS-DSP library for FFT analysis.  
**Alternatives considered**: STM32H7 (higher performance but overkill), External DSP chip (TI TMS320 - adds complexity)

### Angular WebSocket Integration
**Decision**: Service-based architecture with RxJS throttling  
**Rationale**: RxJS provides throttling (16.67ms for 60Hz), automatic reconnection, and robust error handling for real-time RF measurements display.  
**Alternatives considered**: Native WebSocket API (lacks advanced features), Socket.io (overkill for point-to-point)

### RF Measurement Techniques
**Decision**: FFT-based analysis with calibrated RMS power measurement  
**Rationale**: 
- Center frequency: FFT peak detection with noise floor compensation
- Power: True RMS measurement over signal bandwidth  
- SNR: Signal power / noise floor calculation
- ModCod: DVB-S2/S2X frame analysis with PLSCODE detection  
**Alternatives considered**: Time-domain analysis (less accurate), Spectrum analyzer ICs (expensive)

### Netconf Implementation
**Decision**: libnetconf2 with C++ wrappers  
**Rationale**: Mature C library with active development, integrated with sysrepo datastore. More reliable than custom C++ implementations.  
**Alternatives considered**: Custom C++ netconf (high development overhead), Python netconf (performance concerns)

### ADC Architecture 
**Decision**: External RF front-end with down-conversion to baseband  
**Rationale**: STM32F4 ADC limited to 2.4 MSPS, cannot directly sample L-Band. RF front-end down-converts to baseband frequencies suitable for STM32F4 processing.  
**Alternatives considered**: Direct L-Band sampling (impossible), Higher-speed ADCs (cost/complexity increase)

## Key Technical Constraints Identified

1. **Real-time Processing**: 60Hz measurement updates require careful system architecture
2. **RF Front-end Required**: L-Band frequencies exceed STM32F4 direct sampling capability  
3. **Signal Integrity**: FR-014 specifications require precision analog design
4. **Memory Constraints**: Embedded system requires optimized data structures and algorithms
5. **Network Reliability**: WebSocket reconnection and error handling critical for remote monitoring

## Implementation Architecture

```
L-Band RF Signal → RF Front-end → Down-converter → Anti-alias Filter → STM32F4 ADC → DSP Processing → SBC Communication → WebSocket API → Angular Dashboard
```

All unknowns from Technical Context resolved. Ready for Phase 1 design.