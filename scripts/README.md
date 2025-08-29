# Firmware Signing and Verification Utilities

This directory contains utilities for signing and verifying firmware binaries used with the STM32F4 secure bootloader.

## Prerequisites

Install the required Python dependencies:

```bash
pip install cryptography
```

## Utilities

### 1. `firmware_sign.py` - Firmware Signing Tool

Signs firmware binaries with Ed25519 signatures and creates secure firmware headers.

#### Generate Keypair

```bash
./firmware_sign.py keygen --private-key signing_key.pem --public-key public_key.pem
```

This creates:
- `signing_key.pem` - Private key (keep secure, never commit to git!)
- `public_key.pem` - Public key in PEM format  
- `public_key.bin` - Public key in binary format for bootloader

#### Sign Firmware

```bash
./firmware_sign.py sign \
    --firmware application.bin \
    --private-key signing_key.pem \
    --output signed_firmware.bin \
    --version 1 \
    --rollback-counter 1
```

Parameters:
- `--firmware`: Input raw firmware binary
- `--private-key`: Private key file (.pem)
- `--output`: Output signed firmware file
- `--version`: Firmware version number (default: 1)
- `--rollback-counter`: Anti-rollback counter (default: 1)

### 2. `firmware_verify.py` - Firmware Verification Tool

Verifies signed firmware binaries and validates their integrity.

#### Verify Signed Firmware

```bash
./firmware_verify.py verify \
    --firmware signed_firmware.bin \
    --public-key public_key.pem
```

This checks:
- ✅ Magic number validity
- ✅ Header CRC integrity  
- ✅ Firmware size consistency
- ✅ CRC32 checksum
- ✅ SHA-256 hash
- ✅ Ed25519 signature (if public key provided)

#### Extract Raw Firmware

```bash
./firmware_verify.py extract \
    --signed-firmware signed_firmware.bin \
    --output extracted_firmware.bin
```

#### Show Firmware Information

```bash
./firmware_verify.py info --firmware signed_firmware.bin
```

## Security Considerations

### Private Key Management

**Critical**: The private key (`signing_key.pem`) must be kept secure:

1. **Never commit private keys to version control**
2. Store in secure key management systems
3. Use proper file permissions (600)
4. Consider hardware security modules (HSM) for production

### Production vs Development

For development:
- Generate test keypairs locally
- Use version control for public keys only

For production:
- Use secure build environments
- Implement proper key rotation procedures
- Consider code signing infrastructure

## Firmware Header Structure

The secure firmware header contains:

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
    uint8_t reserved[16];              // Reserved space
    uint32_t header_crc;               // Header CRC32
} SecureFirmwareHeader;
```

## A/B Firmware Bank Usage

For A/B firmware updates:

1. **Initial deployment**: Sign and flash firmware to Bank A
2. **Update process**:
   ```bash
   # Sign new firmware version
   ./firmware_sign.py sign --firmware app_v2.bin --output signed_v2.bin --version 2
   
   # Flash to inactive bank (B) via bootloader protocol
   # Bootloader will verify and activate new firmware
   ```

3. **Rollback protection**: Increment `--rollback-counter` to prevent downgrades

## Integration with Build System

Add to your CMakeLists.txt or Makefile:

```cmake
# Sign firmware after build
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/firmware_sign.py sign
        --firmware $<TARGET_FILE:firmware>
        --private-key ${SIGNING_KEY_PATH}
        --output ${CMAKE_BINARY_DIR}/signed_firmware.bin
        --version ${FIRMWARE_VERSION}
    COMMENT "Signing firmware with secure boot header"
)
```

## Example Workflow

Complete signing and verification workflow:

```bash
# 1. Generate keypair (once)
./firmware_sign.py keygen --private-key dev_key.pem --public-key dev_public.pem

# 2. Build your firmware (creates application.bin)
make firmware

# 3. Sign the firmware
./firmware_sign.py sign \
    --firmware build/application.bin \
    --private-key dev_key.pem \
    --output build/signed_firmware.bin \
    --version 1

# 4. Verify before deployment
./firmware_verify.py verify \
    --firmware build/signed_firmware.bin \
    --public-key dev_public.pem

# 5. Flash to device (Bank A or B)
# Use your preferred flashing tool or bootloader protocol
```

## Troubleshooting

### Common Issues

1. **Invalid signature**: Check that public/private key pair matches
2. **CRC mismatch**: Ensure firmware binary is not corrupted
3. **Size mismatch**: Verify firmware file is complete
4. **Magic number invalid**: File may not be properly signed

### Debug Mode

For detailed verification output, use the verification tool which shows:
- Header contents
- Calculated vs expected checksums  
- Step-by-step validation results

### Integration Testing

Test the complete secure boot flow:

1. Flash signed firmware to device
2. Boot and verify secure boot accepts firmware
3. Test A/B bank switching
4. Verify rollback protection works
5. Test recovery mode functionality