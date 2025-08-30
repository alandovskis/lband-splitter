# STM32F4 Secure Boot System

This document provides comprehensive documentation for the secure boot implementation in the STM32F4 firmware.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Security Features](#security-features)
- [Implementation Details](#implementation-details)
- [Build System Integration](#build-system-integration)
- [Deployment Guide](#deployment-guide)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [Security Considerations](#security-considerations)

## Overview

The STM32F4 secure boot system provides cryptographic verification and safe over-the-air (OTA) firmware updates using an A/B banking approach. This ensures firmware integrity, prevents rollback attacks, and provides automatic recovery from failed updates.

### Key Benefits

- **Cryptographic Security**: Ed25519 signatures and SHA-256 hashing
- **Atomic Updates**: A/B firmware banking prevents bricked devices
- **Rollback Protection**: Version-based anti-rollback mechanism
- **Automatic Recovery**: Intelligent fallback and recovery modes
- **Production Ready**: Complete toolchain for development and production

## Architecture

### Memory Layout

```
Flash Memory (1MB total):
├── 0x08000000 - 0x08008000 (32KB)   │ Secure Bootloader
├── 0x08007E00 - 0x08008000 (512B)   │ Boot Configuration
├── 0x08007F00 - 0x08008000 (256B)   │ Public Key Storage  
├── 0x08008000 - 0x0807C000 (~496KB) │ Firmware Bank A
└── 0x0807C000 - 0x080F0000 (~496KB) │ Firmware Bank B
```

### Components

**1. Secure Bootloader** (`bootloader_main.c`, `secure_boot.c`)
- Verifies firmware signatures using Ed25519
- Manages A/B bank selection and switching
- Implements recovery mode and failure tracking
- Provides hardware security enforcement

**2. Firmware Signing Tools** (`scripts/`)
- `firmware_sign.py`: Signs firmware binaries with secure headers
- `firmware_verify.py`: Verifies signed firmware integrity
- Ed25519 key generation and management

**3. Build System Integration** (`CMakeLists.txt`)
- Automated signing pipeline
- Development and production targets
- Configuration management

## Security Features

### Cryptographic Protection

**Ed25519 Digital Signatures**
- 256-bit elliptic curve cryptography
- Fast verification with small signature size (64 bytes)
- Quantum-resistant cryptographic algorithm
- Industry-standard implementation

**SHA-256 Integrity Hashing**
- 256-bit cryptographic hash function
- Detects any firmware modification
- NIST-approved hash algorithm
- Computed over entire firmware payload

**CRC32 Fast Validation**
- 32-bit cyclic redundancy check
- Fast integrity verification for headers
- Detects transmission errors and corruption
- Used for boot configuration protection

### Hardware Security Features

**Flash Memory Protection**
```c
// Enable flash write protection
void secure_boot_enable_flash_protection(void) {
    FLASH->OPTCR |= FLASH_OPTCR_WRP_0;
}
```

**JTAG Debug Disable**
```c
// Disable JTAG in production builds
void secure_boot_disable_jtag_debug(void) {
    #ifndef SECURE_BOOT_ENABLE_DEBUG
    DBGMCU->CR = 0;
    #endif
}
```

**Memory Protection Unit (MPU)**
```c
// Configure memory regions with appropriate permissions
void secure_boot_configure_mpu(void);
```

### Anti-Tampering Mechanisms

**Rollback Protection**
- Firmware version counters prevent downgrades
- Configurable anti-rollback threshold
- Cryptographically signed version information

**Boot Failure Tracking**
- Automatic failure counting per firmware bank
- Configurable failure threshold (default: 3 attempts)
- Persistent failure state across power cycles

**Configuration Integrity**
- CRC-protected boot configuration
- Atomic configuration updates
- Fallback to defaults on corruption

## Implementation Details

### Firmware Header Structure

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;                     // 0x53454342 ("SECB")
    uint32_t version;                   // Firmware version
    uint32_t size;                      // Total firmware size
    uint32_t crc32;                     // CRC32 of payload
    uint8_t sha256_hash[32];           // SHA-256 hash
    uint8_t signature[64];             // Ed25519 signature  
    uint32_t timestamp;                // Build timestamp
    uint32_t rollback_counter;         // Anti-rollback counter
    uint8_t reserved[16];              // Future expansion
    uint32_t header_crc;               // Header CRC32
} SecureFirmwareHeader;
```

### Boot Configuration Structure

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;                    // Boot config magic
    FirmwareBankId active_bank;        // Currently active bank
    FirmwareBankId pending_bank;       // Pending update bank
    uint32_t bank_a_version;           // Bank A version
    uint32_t bank_b_version;           // Bank B version  
    uint32_t bank_a_boot_count;        // Successful boots
    uint32_t bank_b_boot_count;        // Successful boots
    uint32_t bank_a_fail_count;        // Consecutive failures
    uint32_t bank_b_fail_count;        // Consecutive failures
    bool update_in_progress;           // Atomic update flag
    uint32_t rollback_counter;         // Global rollback counter
    uint8_t reserved[32];              // Future expansion
    uint32_t config_crc;               // Structure CRC
} BootConfiguration;
```

### Bank Selection Algorithm

```c
SecureBootResult secure_boot_select_boot_bank(FirmwareBankId* selected_bank) {
    // 1. Load boot configuration from flash
    BootConfiguration config;
    secure_boot_load_boot_config(&config);
    
    // 2. Check for pending update
    if (config.update_in_progress && config.pending_bank != INVALID) {
        *selected_bank = config.pending_bank;
        return SECURE_BOOT_OK;
    }
    
    // 3. Try active bank if not failing
    if (get_failure_count(config.active_bank) < MAX_FAILED_BOOTS) {
        if (secure_boot_is_bank_valid(config.active_bank)) {
            *selected_bank = config.active_bank;
            return SECURE_BOOT_OK;
        }
    }
    
    // 4. Try fallback bank
    FirmwareBankId fallback = (config.active_bank == BANK_A) ? BANK_B : BANK_A;
    if (get_failure_count(fallback) < MAX_FAILED_BOOTS) {
        if (secure_boot_is_bank_valid(fallback)) {
            *selected_bank = fallback;
            return SECURE_BOOT_OK;
        }
    }
    
    // 5. Both banks failed
    return SECURE_BOOT_ERR_FLASH_ERROR;
}
```

## Build System Integration

### CMake Configuration

```cmake
# Secure boot configuration variables
set(FIRMWARE_VERSION "1" CACHE STRING "Firmware version")
set(ROLLBACK_COUNTER "1" CACHE STRING "Rollback counter")
set(SIGNING_KEY_PATH "" CACHE FILEPATH "Private signing key")
set(PUBLIC_KEY_PATH "" CACHE FILEPATH "Public verification key")

# Python script integration
find_package(Python3 COMPONENTS Interpreter)
set(FIRMWARE_SIGN_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/firmware_sign.py")
set(FIRMWARE_VERIFY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/firmware_verify.py")
```

### Available Build Targets

| Target | Description |
|--------|-------------|
| `generate_keys` | Generate Ed25519 signing keypair |
| `dev_setup` | Setup complete development environment |
| `sign_firmware` | Sign firmware with secure headers |
| `verify_firmware` | Verify signed firmware integrity |
| `flash_bootloader` | Flash bootloader to 0x8000000 |
| `flash_signed_a` | Flash signed firmware to Bank A |
| `flash_signed_b` | Flash signed firmware to Bank B |
| `flash_complete_system` | Flash bootloader + signed firmware |
| `firmware_info` | Show signed firmware information |
| `help_secure_boot` | Display all secure boot targets |

### Configuration Example

```bash
# Development setup
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSIGNING_KEY_PATH=dev_key.pem \
      -DPUBLIC_KEY_PATH=dev_pub.pem \
      -DFIRMWARE_VERSION=1 \
      .

# Production setup  
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSIGNING_KEY_PATH=prod_key.pem \
      -DPUBLIC_KEY_PATH=prod_pub.pem \
      -DFIRMWARE_VERSION=5 \
      -DROLLBACK_COUNTER=5 \
      .
```

## Deployment Guide

### Development Workflow

**1. Initial Setup**
```bash
# Generate development keys
make generate_keys

# Configure build system
cmake -DSIGNING_KEY_PATH=firmware_signing_key.pem \
      -DPUBLIC_KEY_PATH=firmware_public_key.pem .

# Setup development environment
make dev_setup
```

**2. Build and Test**
```bash
# Build both bootloader and firmware
make stm32f4_frequency_detector_bootloader
make stm32f4_frequency_detector

# Sign and verify firmware
make sign_firmware
make verify_firmware

# Show firmware information
make firmware_info
```

**3. Flash Development System**
```bash
# Flash complete system
make flash_complete_system

# Or flash components separately
make flash_bootloader
make flash_signed_a
```

### Production Deployment

**1. Key Management**
```bash
# Generate production keys (secure environment)
./scripts/firmware_sign.py keygen \
    --private-key production_private.pem \
    --public-key production_public.pem

# Store private key securely (HSM, secure storage)
# Distribute public key to devices
```

**2. Production Build**
```bash
# Configure for production
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSIGNING_KEY_PATH=/secure/path/production_private.pem \
      -DPUBLIC_KEY_PATH=production_public.pem \
      -DFIRMWARE_VERSION=1 \
      .

# Build and sign
make stm32f4_frequency_detector_bootloader
make sign_firmware
make verify_firmware
```

**3. Manufacturing Flash**
```bash
# Flash bootloader (one-time)
make flash_bootloader

# Flash initial firmware to Bank A
make flash_signed_a

# Or use external programmer
st-flash write stm32f4_frequency_detector_bootloader.bin 0x8000000
st-flash write stm32f4_frequency_detector_signed.bin 0x8008000
```

### Over-the-Air Updates

**1. Prepare Update**
```bash
# Increment version for new release
cmake -DFIRMWARE_VERSION=2 -DROLLBACK_COUNTER=2 .

# Build and sign new firmware
make stm32f4_frequency_detector
make sign_firmware
make verify_firmware
```

**2. Deploy to Inactive Bank**
```bash
# If Bank A is active, deploy to Bank B
make flash_signed_b

# Or via OTA protocol
# (Implementation-specific OTA mechanism)
```

**3. Automatic Bank Switching**
- Bootloader detects new firmware in inactive bank
- Verifies signature and integrity
- Switches to new bank on next boot
- Falls back to previous bank if new firmware fails

## API Reference

### Core Functions

```c
// Initialize secure boot system
SecureBootResult secure_boot_init(void);

// Verify firmware signature and integrity
SecureBootResult secure_boot_verify_firmware(uint32_t firmware_addr);

// Jump to verified application
SecureBootResult secure_boot_jump_to_application(uint32_t app_addr);

// Enter emergency recovery mode
void secure_boot_emergency_recovery(void);
```

### A/B Bank Management

```c
// Load/save boot configuration
SecureBootResult secure_boot_load_boot_config(BootConfiguration* config);
SecureBootResult secure_boot_save_boot_config(const BootConfiguration* config);

// Bank selection and validation
SecureBootResult secure_boot_select_boot_bank(FirmwareBankId* selected_bank);
bool secure_boot_is_bank_valid(FirmwareBankId bank);
uint32_t secure_boot_get_bank_address(FirmwareBankId bank);

// Firmware update operations
SecureBootResult secure_boot_begin_update(FirmwareBankId target_bank);
SecureBootResult secure_boot_commit_update(FirmwareBankId updated_bank);
SecureBootResult secure_boot_rollback_update(void);
```

### Utility Functions

```c
// Status tracking
SecureBootResult secure_boot_mark_bank_successful(FirmwareBankId bank);
SecureBootResult secure_boot_mark_bank_failed(FirmwareBankId bank);
uint32_t secure_boot_get_bank_boot_count(FirmwareBankId bank);

// Information and debugging
const char* secure_boot_result_to_string(SecureBootResult result);
const char* secure_boot_bank_to_string(FirmwareBankId bank);
void secure_boot_print_firmware_info(const SecureFirmwareHeader* header);
void secure_boot_print_boot_config(const BootConfiguration* config);
```

## Troubleshooting

### Common Issues

**Signature Verification Failed**
- **Cause**: Public/private key mismatch
- **Solution**: Verify key pair integrity, regenerate if needed
- **Debug**: Check signature in firmware header vs. expected

**Invalid Firmware Header**  
- **Cause**: Unsigned firmware or corrupt header
- **Solution**: Re-sign firmware, verify signing process
- **Debug**: Check magic number (0x53454342) and header CRC

**Bank Selection Failed**
- **Cause**: Both firmware banks invalid or corrupted
- **Solution**: Flash known-good signed firmware to at least one bank
- **Debug**: Check bank validity with `secure_boot_is_bank_valid()`

**Rollback Protection Triggered**
- **Cause**: Attempting to install older firmware version
- **Solution**: Increment version and rollback counter appropriately
- **Debug**: Check version in firmware header vs. stored rollback counter

### Recovery Procedures

**Manual Recovery Mode**
```c
// Force entry to recovery mode
if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
    secure_boot_emergency_recovery();
}
```

**Flash Recovery**
```bash
# Re-flash bootloader (if corrupted)
st-flash write stm32f4_frequency_detector_bootloader.bin 0x8000000

# Flash known-good firmware to Bank A
st-flash write working_signed_firmware.bin 0x8008000

# Clear boot configuration (force defaults)
st-flash erase 0x08007E00 512
```

**Debug Information**
```bash
# Show detailed firmware information
../../scripts/firmware_verify.py info --firmware signed_firmware.bin

# Verify signature manually
../../scripts/firmware_verify.py verify --firmware signed_firmware.bin --public-key public_key.pem

# Extract raw firmware for analysis
../../scripts/firmware_verify.py extract --signed-firmware signed_firmware.bin --output raw.bin
```

### LED Status Indicators

The bootloader provides visual feedback through LED status codes:

| Pattern | Meaning |
|---------|---------|
| 3 fast blinks | Secure boot initialization error |
| 5 very fast blinks | Recovery mode active |
| 8 medium blinks | Bank selection failed |
| 9 blinks | Invalid bank address |
| 10 slow blinks | Firmware verification failed |
| 20 rapid blinks | Jump to application failed |
| Solid on | Firmware verification in progress |
| Off | Normal operation |

## Security Considerations

### Key Management

**Development Keys**
- Use separate keys for development and production
- Store development keys in version control (public key only)
- Rotate development keys regularly

**Production Keys**
- Generate keys in secure environment (air-gapped system)
- Store private keys in Hardware Security Modules (HSM)
- Implement key rotation procedures
- Use code signing infrastructure for large deployments

**Key Distribution**
- Embed public keys in bootloader during manufacturing
- Use secure provisioning for public key updates
- Implement key revocation mechanisms if needed

### Threat Model

**Protected Against:**
- Unauthorized firmware installation
- Firmware tampering and modification
- Rollback attacks to vulnerable versions
- Bricking from failed updates
- Flash memory corruption

**Attack Considerations:**
- Physical access to device (secure bootloader and flash protection)
- Side-channel attacks (timing, power analysis)
- Key extraction attempts (secure key storage)
- Social engineering (secure development practices)

### Best Practices

**Development**
- Use debug builds only in development
- Disable JTAG in production builds
- Implement secure boot from early development
- Test recovery scenarios thoroughly

**Production**
- Use Hardware Security Modules for key storage
- Implement secure manufacturing processes
- Monitor firmware update success rates
- Maintain firmware signing audit trails

**Deployment**
- Verify signatures before any firmware installation
- Implement graduated rollout for firmware updates
- Monitor device health and boot success rates
- Maintain firmware version inventory

## Conclusion

The STM32F4 secure boot system provides enterprise-grade security for firmware updates while maintaining ease of development and deployment. The A/B banking approach ensures reliable updates, while cryptographic verification prevents unauthorized firmware installation.

For additional support and examples, see:
- `scripts/README.md` - Firmware signing utilities documentation
- `src/firmware/stm32f4/README.md` - Complete firmware documentation
- `secure_boot.h` - API reference and data structures
- `bootloader_main.c` - Bootloader implementation example