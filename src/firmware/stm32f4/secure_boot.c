#include "secure_boot.h"
#include <string.h>
#include <stdio.h>

// Global secure boot context
static SecureBootContext g_secure_boot_ctx = {0};

// Initialize default boot configuration
static void init_default_boot_config(BootConfiguration* config) {
    config->magic = BOOT_CONFIG_MAGIC;
    config->active_bank = FIRMWARE_BANK_A;
    config->pending_bank = FIRMWARE_BANK_INVALID;
    config->bank_a_version = 0;
    config->bank_b_version = 0;
    config->bank_a_boot_count = 0;
    config->bank_b_boot_count = 0;
    config->bank_a_fail_count = 0;
    config->bank_b_fail_count = 0;
    config->update_in_progress = false;
    config->rollback_counter = 0;
    memset(config->reserved, 0, sizeof(config->reserved));
    config->config_crc = secure_boot_calculate_crc32((uint8_t*)config, 
                                                     sizeof(*config) - sizeof(config->config_crc));
}

// CRC32 lookup table
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// Simple Ed25519 signature verification (placeholder - in production use a proper crypto library)
static bool simple_ed25519_verify(const uint8_t* message, size_t message_len,
                                 const uint8_t* signature, const uint8_t* public_key) {
    // This is a simplified placeholder implementation
    // In production, use a proper Ed25519 library like libsodium or mbedTLS
    
    // For demonstration purposes, we'll do a simple checksum verification
    // DO NOT use this in production!
    uint32_t message_hash = secure_boot_calculate_crc32(message, message_len);
    uint32_t sig_check = 0;
    
    for (int i = 0; i < 32; i++) {
        sig_check ^= public_key[i] << (i % 24);
    }
    
    for (int i = 0; i < 32; i++) {
        sig_check ^= signature[i] << ((i + 16) % 24);
    }
    
    // Simple verification - replace with real Ed25519
    return (message_hash & 0xFFFF) == (sig_check & 0xFFFF);
}

// Initialize secure boot system
SecureBootResult secure_boot_init(void) {
    memset(&g_secure_boot_ctx, 0, sizeof(g_secure_boot_ctx));
    
    g_secure_boot_ctx.state = SECURE_BOOT_STATE_UNINITIALIZED;
    g_secure_boot_ctx.development_mode = SECURE_BOOT_ENABLE_DEVELOPMENT_MODE;
    g_secure_boot_ctx.debug_enabled = SECURE_BOOT_ENABLE_DEBUG;
    
    // Load public key from flash
    SecureBootResult result = secure_boot_load_public_key(&g_secure_boot_ctx.public_key);
    if (result != SECURE_BOOT_OK) {
        secure_boot_debug_print("Failed to load public key: %s\n", 
                               secure_boot_result_to_string(result));
        return result;
    }
    
    // Validate public key
    result = secure_boot_validate_public_key(&g_secure_boot_ctx.public_key);
    if (result != SECURE_BOOT_OK) {
        secure_boot_debug_print("Public key validation failed: %s\n",
                               secure_boot_result_to_string(result));
        return result;
    }
    
    g_secure_boot_ctx.state = SECURE_BOOT_STATE_VERIFYING;
    secure_boot_increment_boot_counter();
    
    secure_boot_debug_print("Secure boot initialized successfully\n");
    return SECURE_BOOT_OK;
}

// Verify firmware signature and integrity
SecureBootResult secure_boot_verify_firmware(uint32_t firmware_addr) {
    SecureFirmwareHeader header;
    
    // Read firmware header
    SecureBootResult result = secure_boot_read_flash(firmware_addr, 
                                                    (uint8_t*)&header, 
                                                    sizeof(header));
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Validate header magic
    if (header.magic != SECURE_BOOT_MAGIC) {
        secure_boot_debug_print("Invalid firmware magic: 0x%08X\n", header.magic);
        return SECURE_BOOT_ERR_INVALID_HEADER;
    }
    
    // Check header CRC
    uint32_t header_crc = secure_boot_calculate_crc32((uint8_t*)&header, 
                                                     sizeof(header) - sizeof(header.header_crc));
    if (header_crc != header.header_crc) {
        secure_boot_debug_print("Header CRC mismatch\n");
        return SECURE_BOOT_ERR_INVALID_HEADER;
    }
    
    // Check firmware size
    if (header.size > APPLICATION_MAX_SIZE) {
        secure_boot_debug_print("Firmware size too large: %u bytes\n", header.size);
        return SECURE_BOOT_ERR_INVALID_HEADER;
    }
    
    // Check rollback protection if enabled
    if (SECURE_BOOT_ENABLE_ROLLBACK_PROTECTION) {
        result = secure_boot_check_version(header.rollback_counter);
        if (result != SECURE_BOOT_OK) {
            return result;
        }
    }
    
    // Calculate firmware hash
    uint8_t calculated_hash[SECURE_BOOT_HASH_SIZE];
    uint32_t firmware_data_addr = firmware_addr + sizeof(SecureFirmwareHeader);
    
    // Read firmware data in chunks and calculate hash
    // This is a simplified implementation - use proper SHA-256 in production
    uint32_t running_crc = 0xFFFFFFFF;
    uint8_t buffer[256];
    uint32_t remaining = header.size - sizeof(SecureFirmwareHeader);
    uint32_t offset = 0;
    
    while (remaining > 0) {
        uint32_t chunk_size = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        
        result = secure_boot_read_flash(firmware_data_addr + offset, buffer, chunk_size);
        if (result != SECURE_BOOT_OK) {
            return result;
        }
        
        // Update running CRC (simplified hash)
        for (uint32_t i = 0; i < chunk_size; i++) {
            running_crc = crc32_table[(running_crc ^ buffer[i]) & 0xFF] ^ (running_crc >> 8);
        }
        
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    // Finalize hash calculation (simplified)
    running_crc ^= 0xFFFFFFFF;
    memset(calculated_hash, 0, sizeof(calculated_hash));
    memcpy(calculated_hash, &running_crc, sizeof(running_crc));
    
    // Verify hash matches header
    if (memcmp(calculated_hash, header.sha256_hash, 4) != 0) { // Simplified comparison
        secure_boot_debug_print("Firmware hash mismatch\n");
        return SECURE_BOOT_ERR_HASH_MISMATCH;
    }
    
    // Verify signature
    result = secure_boot_verify_signature((uint8_t*)&header, 
                                         sizeof(header) - SECURE_BOOT_SIGNATURE_SIZE,
                                         header.signature,
                                         &g_secure_boot_ctx.public_key);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Store verified firmware header
    memcpy(&g_secure_boot_ctx.firmware_header, &header, sizeof(header));
    g_secure_boot_ctx.state = SECURE_BOOT_STATE_VERIFIED;
    
    secure_boot_debug_print("Firmware verification successful\n");
    secure_boot_print_firmware_info(&header);
    
    return SECURE_BOOT_OK;
}

// Jump to verified application
SecureBootResult secure_boot_jump_to_application(uint32_t app_addr) {
    if (g_secure_boot_ctx.state != SECURE_BOOT_STATE_VERIFIED) {
        return SECURE_BOOT_ERR_INVALID_SIGNATURE;
    }
    
    secure_boot_debug_print("Jumping to application at 0x%08X\n", app_addr);
    
    // Disable interrupts
    __disable_irq();
    
    // Configure MPU for application protection
    secure_boot_configure_mpu();
    
    // Set up stack pointer and jump to application
    uint32_t* app_vector_table = (uint32_t*)app_addr;
    uint32_t app_stack_pointer = app_vector_table[0];
    uint32_t app_entry_point = app_vector_table[1];
    
    // Validate application entry point
    if (app_entry_point < app_addr || app_entry_point >= (app_addr + APPLICATION_MAX_SIZE)) {
        return SECURE_BOOT_ERR_INVALID_HEADER;
    }
    
    // Set stack pointer and jump
    __set_MSP(app_stack_pointer);
    
    // Create function pointer and call application
    void (*app_main)(void) = (void (*)(void))app_entry_point;
    app_main();
    
    // Should never reach here
    return SECURE_BOOT_ERR_FLASH_ERROR;
}

// Emergency recovery mode
void secure_boot_emergency_recovery(void) {
    secure_boot_debug_print("Entering emergency recovery mode\n");
    
    g_secure_boot_ctx.state = SECURE_BOOT_STATE_FAILED;
    secure_boot_record_boot_failure();
    
    // Flash LEDs to indicate recovery mode
    for (int i = 0; i < 10; i++) {
        // Toggle LED - implementation depends on hardware
        HAL_Delay(200);
    }
    
    // Wait for recovery command via UART
    // Implementation would depend on recovery protocol
    while (1) {
        HAL_Delay(1000);
        secure_boot_debug_print("Recovery mode active - waiting for commands\n");
    }
}

// Signature verification
SecureBootResult secure_boot_verify_signature(const uint8_t* data, uint32_t data_len,
                                             const uint8_t* signature,
                                             const SecurePublicKey* public_key) {
    if (!data || !signature || !public_key) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    // Verify Ed25519 signature
    bool valid = simple_ed25519_verify(data, data_len, signature, public_key->key_data);
    
    if (!valid) {
        secure_boot_debug_print("Signature verification failed\n");
        return SECURE_BOOT_ERR_INVALID_SIGNATURE;
    }
    
    return SECURE_BOOT_OK;
}

// Calculate SHA-256 hash (simplified implementation)
SecureBootResult secure_boot_calculate_hash(const uint8_t* data, uint32_t data_len,
                                           uint8_t* hash_output) {
    if (!data || !hash_output) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    // Simplified hash calculation using CRC32
    // In production, use proper SHA-256 implementation
    uint32_t crc = secure_boot_calculate_crc32(data, data_len);
    memset(hash_output, 0, SECURE_BOOT_HASH_SIZE);
    memcpy(hash_output, &crc, sizeof(crc));
    
    return SECURE_BOOT_OK;
}

// Calculate CRC32
uint32_t secure_boot_calculate_crc32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Flash read operation
SecureBootResult secure_boot_read_flash(uint32_t address, uint8_t* buffer, uint32_t length) {
    if (!buffer || length == 0) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Validate address range
    if (address < BOOTLOADER_START_ADDR || 
        (address + length) > (BOOTLOADER_START_ADDR + 1024*1024)) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Direct memory read from flash
    memcpy(buffer, (void*)address, length);
    
    return SECURE_BOOT_OK;
}

// Flash write operation (simplified implementation)
SecureBootResult secure_boot_write_flash(uint32_t address, const uint8_t* data, uint32_t length) {
    if (!data || length == 0) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Validate address range (only allow writes to configuration areas)
    if (address < BOOT_CONFIG_FLASH_ADDR || 
        address >= (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE)) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Unlock flash for writing
    HAL_StatusTypeDef hal_result = HAL_FLASH_Unlock();
    if (hal_result != HAL_OK) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Erase the sector first (simplified - assumes single sector)
    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = FLASH_SECTOR_1; // Bootloader configuration sector
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    uint32_t sector_error = 0;
    hal_result = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (hal_result != HAL_OK) {
        HAL_FLASH_Lock();
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Write data in 32-bit words
    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word_data = 0;
        uint32_t bytes_to_copy = (length - i < 4) ? (length - i) : 4;
        memcpy(&word_data, &data[i], bytes_to_copy);
        
        hal_result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word_data);
        if (hal_result != HAL_OK) {
            HAL_FLASH_Lock();
            return SECURE_BOOT_ERR_FLASH_ERROR;
        }
    }
    
    // Lock flash
    HAL_FLASH_Lock();
    
    return SECURE_BOOT_OK;
}

// Load public key from flash
SecureBootResult secure_boot_load_public_key(SecurePublicKey* key) {
    if (!key) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    SecureBootResult result = secure_boot_read_flash(PUBLIC_KEY_FLASH_ADDR,
                                                    (uint8_t*)key,
                                                    sizeof(SecurePublicKey));
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    return secure_boot_validate_public_key(key);
}

// Validate public key
SecureBootResult secure_boot_validate_public_key(const SecurePublicKey* key) {
    if (!key) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    // Check key CRC
    uint32_t calculated_crc = secure_boot_calculate_crc32(key->key_data,
                                                         sizeof(key->key_data) + 
                                                         sizeof(key->key_version) +
                                                         sizeof(key->key_flags) +
                                                         sizeof(key->key_id));
    
    if (calculated_crc != key->key_crc) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    // Check if key is marked for firmware signing
    if (!(key->key_flags & SECURE_BOOT_KEY_FLAG_FIRMWARE_SIGN)) {
        return SECURE_BOOT_ERR_CRYPTO_ERROR;
    }
    
    return SECURE_BOOT_OK;
}

// Version and rollback protection
SecureBootResult secure_boot_check_version(uint32_t firmware_version) {
    uint32_t stored_counter = secure_boot_get_rollback_counter();
    
    if (firmware_version < stored_counter) {
        secure_boot_debug_print("Rollback protection triggered: fw=%u, stored=%u\n",
                               firmware_version, stored_counter);
        return SECURE_BOOT_ERR_ROLLBACK_PROTECTION;
    }
    
    return SECURE_BOOT_OK;
}

// Get rollback counter (stored in flash or backup SRAM)
uint32_t secure_boot_get_rollback_counter(void) {
    // Simplified implementation - would typically be in secure storage
    return 1; // Default version
}

// Utility functions
const char* secure_boot_result_to_string(SecureBootResult result) {
    switch (result) {
        case SECURE_BOOT_OK: return "OK";
        case SECURE_BOOT_ERR_INVALID_HEADER: return "Invalid Header";
        case SECURE_BOOT_ERR_INVALID_SIGNATURE: return "Invalid Signature";
        case SECURE_BOOT_ERR_HASH_MISMATCH: return "Hash Mismatch";
        case SECURE_BOOT_ERR_INVALID_VERSION: return "Invalid Version";
        case SECURE_BOOT_ERR_FLASH_ERROR: return "Flash Error";
        case SECURE_BOOT_ERR_CRYPTO_ERROR: return "Crypto Error";
        case SECURE_BOOT_ERR_ROLLBACK_PROTECTION: return "Rollback Protection";
        default: return "Unknown Error";
    }
}

const char* secure_boot_state_to_string(SecureBootState state) {
    switch (state) {
        case SECURE_BOOT_STATE_UNINITIALIZED: return "Uninitialized";
        case SECURE_BOOT_STATE_VERIFYING: return "Verifying";
        case SECURE_BOOT_STATE_VERIFIED: return "Verified";
        case SECURE_BOOT_STATE_FAILED: return "Failed";
        case SECURE_BOOT_STATE_ROLLBACK: return "Rollback";
        default: return "Unknown State";
    }
}

void secure_boot_print_firmware_info(const SecureFirmwareHeader* header) {
    if (!header) return;
    
    secure_boot_debug_print("=== Firmware Information ===\n");
    secure_boot_debug_print("Version: %u\n", header->version);
    secure_boot_debug_print("Size: %u bytes\n", header->size);
    secure_boot_debug_print("CRC32: 0x%08X\n", header->crc32);
    secure_boot_debug_print("Timestamp: %u\n", header->timestamp);
    secure_boot_debug_print("Rollback Counter: %u\n", header->rollback_counter);
}

// Boot statistics
SecureBootContext* secure_boot_get_context(void) {
    return &g_secure_boot_ctx;
}

void secure_boot_increment_boot_counter(void) {
    g_secure_boot_ctx.boot_count++;
}

void secure_boot_record_boot_failure(void) {
    g_secure_boot_ctx.failed_boots++;
    g_secure_boot_ctx.last_result = SECURE_BOOT_ERR_INVALID_SIGNATURE;
}

bool secure_boot_is_recovery_needed(void) {
    return g_secure_boot_ctx.failed_boots >= SECURE_BOOT_MAX_FAILED_BOOTS;
}

// Hardware security features
void secure_boot_enable_flash_protection(void) {
    // Enable flash write protection
    FLASH->OPTCR |= FLASH_OPTCR_WRP_0;
}

void secure_boot_disable_jtag_debug(void) {
    // Disable JTAG debug access in production
    #ifndef SECURE_BOOT_ENABLE_DEBUG
    DBGMCU->CR = 0;
    #endif
}

void secure_boot_configure_mpu(void) {
    // Configure Memory Protection Unit
    // This would set up memory regions with appropriate permissions
    // Implementation depends on specific security requirements
}

// Debug support
void secure_boot_debug_print(const char* format, ...) {
    #if SECURE_BOOT_ENABLE_DEBUG
    // Implementation would output to UART or debug interface
    // Simplified for this example
    (void)format; // Suppress unused warning
    #else
    (void)format;
    #endif
}

void secure_boot_set_development_mode(bool enabled) {
    g_secure_boot_ctx.development_mode = enabled;
}

bool secure_boot_is_development_mode(void) {
    return g_secure_boot_ctx.development_mode;
}

// A/B firmware bank management functions

// Load boot configuration from flash
SecureBootResult secure_boot_load_boot_config(BootConfiguration* config) {
    if (!config) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    SecureBootResult result = secure_boot_read_flash(BOOT_CONFIG_FLASH_ADDR,
                                                    (uint8_t*)config,
                                                    sizeof(BootConfiguration));
    if (result != SECURE_BOOT_OK) {
        secure_boot_debug_print("Failed to read boot config from flash\n");
        init_default_boot_config(config);
        return SECURE_BOOT_OK; // Use defaults on read failure
    }
    
    // Validate configuration
    if (config->magic != BOOT_CONFIG_MAGIC) {
        secure_boot_debug_print("Invalid boot config magic, using defaults\n");
        init_default_boot_config(config);
        return SECURE_BOOT_OK;
    }
    
    // Verify CRC
    uint32_t calculated_crc = secure_boot_calculate_crc32((uint8_t*)config,
                                                         sizeof(*config) - sizeof(config->config_crc));
    if (calculated_crc != config->config_crc) {
        secure_boot_debug_print("Boot config CRC mismatch, using defaults\n");
        init_default_boot_config(config);
        return SECURE_BOOT_OK;
    }
    
    return SECURE_BOOT_OK;
}

// Save boot configuration to flash
SecureBootResult secure_boot_save_boot_config(const BootConfiguration* config) {
    if (!config) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Update CRC before saving
    BootConfiguration temp_config;
    memcpy(&temp_config, config, sizeof(temp_config));
    temp_config.config_crc = secure_boot_calculate_crc32((uint8_t*)&temp_config,
                                                        sizeof(temp_config) - sizeof(temp_config.config_crc));
    
    return secure_boot_write_flash(BOOT_CONFIG_FLASH_ADDR,
                                  (uint8_t*)&temp_config,
                                  sizeof(temp_config));
}

// Select the best boot bank based on priority and health
SecureBootResult secure_boot_select_boot_bank(FirmwareBankId* selected_bank) {
    if (!selected_bank) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Copy config to global context
    memcpy(&g_secure_boot_ctx.boot_config, &config, sizeof(config));
    
    // If there's an update in progress, continue with pending bank
    if (config.update_in_progress && config.pending_bank != FIRMWARE_BANK_INVALID) {
        secure_boot_debug_print("Update in progress, using pending bank %s\n",
                               secure_boot_bank_to_string(config.pending_bank));
        *selected_bank = config.pending_bank;
        return SECURE_BOOT_OK;
    }
    
    // Try active bank first if it's not failing
    FirmwareBankId primary_bank = config.active_bank;
    uint32_t primary_fail_count = (primary_bank == FIRMWARE_BANK_A) ? 
                                  config.bank_a_fail_count : config.bank_b_fail_count;
    
    if (primary_fail_count < SECURE_BOOT_MAX_FAILED_BOOTS) {
        if (secure_boot_is_bank_valid(primary_bank)) {
            *selected_bank = primary_bank;
            return SECURE_BOOT_OK;
        }
    }
    
    // Primary bank failed, try the other bank
    FirmwareBankId fallback_bank = (primary_bank == FIRMWARE_BANK_A) ? 
                                   FIRMWARE_BANK_B : FIRMWARE_BANK_A;
    uint32_t fallback_fail_count = (fallback_bank == FIRMWARE_BANK_A) ? 
                                   config.bank_a_fail_count : config.bank_b_fail_count;
    
    if (fallback_fail_count < SECURE_BOOT_MAX_FAILED_BOOTS) {
        if (secure_boot_is_bank_valid(fallback_bank)) {
            secure_boot_debug_print("Primary bank failed, using fallback bank %s\n",
                                   secure_boot_bank_to_string(fallback_bank));
            *selected_bank = fallback_bank;
            return SECURE_BOOT_OK;
        }
    }
    
    // Both banks failed
    secure_boot_debug_print("Both firmware banks have failed\n");
    return SECURE_BOOT_ERR_FLASH_ERROR;
}

// Get flash address for a firmware bank
uint32_t secure_boot_get_bank_address(FirmwareBankId bank) {
    switch (bank) {
        case FIRMWARE_BANK_A:
            return FIRMWARE_BANK_A_ADDR;
        case FIRMWARE_BANK_B:
            return FIRMWARE_BANK_B_ADDR;
        default:
            return 0;
    }
}

// Verify both firmware banks and determine priority
SecureBootResult secure_boot_verify_both_banks(FirmwareBankId* primary, FirmwareBankId* fallback) {
    if (!primary || !fallback) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    *primary = FIRMWARE_BANK_INVALID;
    *fallback = FIRMWARE_BANK_INVALID;
    
    bool bank_a_valid = false;
    bool bank_b_valid = false;
    uint32_t bank_a_version = 0;
    uint32_t bank_b_version = 0;
    
    // Check Bank A
    uint32_t bank_a_addr = secure_boot_get_bank_address(FIRMWARE_BANK_A);
    if (secure_boot_verify_firmware(bank_a_addr) == SECURE_BOOT_OK) {
        bank_a_valid = true;
        bank_a_version = secure_boot_get_bank_version(FIRMWARE_BANK_A);
    }
    
    // Check Bank B
    uint32_t bank_b_addr = secure_boot_get_bank_address(FIRMWARE_BANK_B);
    if (secure_boot_verify_firmware(bank_b_addr) == SECURE_BOOT_OK) {
        bank_b_valid = true;
        bank_b_version = secure_boot_get_bank_version(FIRMWARE_BANK_B);
    }
    
    // Determine priority based on version and validity
    if (bank_a_valid && bank_b_valid) {
        // Both valid - choose higher version as primary
        if (bank_a_version >= bank_b_version) {
            *primary = FIRMWARE_BANK_A;
            *fallback = FIRMWARE_BANK_B;
        } else {
            *primary = FIRMWARE_BANK_B;
            *fallback = FIRMWARE_BANK_A;
        }
    } else if (bank_a_valid) {
        *primary = FIRMWARE_BANK_A;
    } else if (bank_b_valid) {
        *primary = FIRMWARE_BANK_B;
    } else {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    return SECURE_BOOT_OK;
}

// Begin firmware update process
SecureBootResult secure_boot_begin_update(FirmwareBankId target_bank) {
    if (target_bank != FIRMWARE_BANK_A && target_bank != FIRMWARE_BANK_B) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Set update parameters
    config.pending_bank = target_bank;
    config.update_in_progress = true;
    
    // Save configuration
    result = secure_boot_save_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    secure_boot_debug_print("Update begun for bank %s\n", secure_boot_bank_to_string(target_bank));
    return SECURE_BOOT_OK;
}

// Commit firmware update (make it the active bank)
SecureBootResult secure_boot_commit_update(FirmwareBankId updated_bank) {
    if (updated_bank != FIRMWARE_BANK_A && updated_bank != FIRMWARE_BANK_B) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Verify the updated bank is valid
    if (!secure_boot_is_bank_valid(updated_bank)) {
        return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    // Switch to new active bank
    config.active_bank = updated_bank;
    config.pending_bank = FIRMWARE_BANK_INVALID;
    config.update_in_progress = false;
    
    // Reset failure count for the new active bank
    if (updated_bank == FIRMWARE_BANK_A) {
        config.bank_a_fail_count = 0;
    } else {
        config.bank_b_fail_count = 0;
    }
    
    // Save configuration
    result = secure_boot_save_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    secure_boot_debug_print("Update committed for bank %s\n", secure_boot_bank_to_string(updated_bank));
    return SECURE_BOOT_OK;
}

// Rollback firmware update
SecureBootResult secure_boot_rollback_update(void) {
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Cancel pending update
    config.pending_bank = FIRMWARE_BANK_INVALID;
    config.update_in_progress = false;
    
    // Save configuration
    result = secure_boot_save_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    secure_boot_debug_print("Firmware update rolled back\n");
    return SECURE_BOOT_OK;
}

// Check if firmware update is pending
bool secure_boot_is_update_pending(void) {
    return g_secure_boot_ctx.boot_config.update_in_progress;
}

// Check if a firmware bank is valid
bool secure_boot_is_bank_valid(FirmwareBankId bank) {
    uint32_t bank_addr = secure_boot_get_bank_address(bank);
    if (bank_addr == 0) {
        return false;
    }
    
    return (secure_boot_verify_firmware(bank_addr) == SECURE_BOOT_OK);
}

// Get firmware version for a bank
uint32_t secure_boot_get_bank_version(FirmwareBankId bank) {
    uint32_t bank_addr = secure_boot_get_bank_address(bank);
    if (bank_addr == 0) {
        return 0;
    }
    
    SecureFirmwareHeader header;
    SecureBootResult result = secure_boot_read_flash(bank_addr, (uint8_t*)&header, sizeof(header));
    if (result != SECURE_BOOT_OK || header.magic != SECURE_BOOT_MAGIC) {
        return 0;
    }
    
    return header.version;
}

// Get boot count for a firmware bank
uint32_t secure_boot_get_bank_boot_count(FirmwareBankId bank) {
    const BootConfiguration* config = &g_secure_boot_ctx.boot_config;
    
    switch (bank) {
        case FIRMWARE_BANK_A:
            return config->bank_a_boot_count;
        case FIRMWARE_BANK_B:
            return config->bank_b_boot_count;
        default:
            return 0;
    }
}

// Mark firmware bank as successful
SecureBootResult secure_boot_mark_bank_successful(FirmwareBankId bank) {
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Increment boot count and reset failure count
    switch (bank) {
        case FIRMWARE_BANK_A:
            config.bank_a_boot_count++;
            config.bank_a_fail_count = 0;
            break;
        case FIRMWARE_BANK_B:
            config.bank_b_boot_count++;
            config.bank_b_fail_count = 0;
            break;
        default:
            return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    return secure_boot_save_boot_config(&config);
}

// Mark firmware bank as failed
SecureBootResult secure_boot_mark_bank_failed(FirmwareBankId bank) {
    BootConfiguration config;
    SecureBootResult result = secure_boot_load_boot_config(&config);
    if (result != SECURE_BOOT_OK) {
        return result;
    }
    
    // Increment failure count
    switch (bank) {
        case FIRMWARE_BANK_A:
            config.bank_a_fail_count++;
            break;
        case FIRMWARE_BANK_B:
            config.bank_b_fail_count++;
            break;
        default:
            return SECURE_BOOT_ERR_FLASH_ERROR;
    }
    
    return secure_boot_save_boot_config(&config);
}

// Utility function to convert bank ID to string
const char* secure_boot_bank_to_string(FirmwareBankId bank) {
    switch (bank) {
        case FIRMWARE_BANK_A: return "Bank A";
        case FIRMWARE_BANK_B: return "Bank B";
        case FIRMWARE_BANK_INVALID: return "Invalid";
        default: return "Unknown";
    }
}

// Print boot configuration information
void secure_boot_print_boot_config(const BootConfiguration* config) {
    if (!config) return;
    
    secure_boot_debug_print("=== Boot Configuration ===\n");
    secure_boot_debug_print("Active Bank: %s\n", secure_boot_bank_to_string(config->active_bank));
    secure_boot_debug_print("Pending Bank: %s\n", secure_boot_bank_to_string(config->pending_bank));
    secure_boot_debug_print("Update In Progress: %s\n", config->update_in_progress ? "Yes" : "No");
    secure_boot_debug_print("Bank A Version: %u (Boots: %u, Failures: %u)\n", 
                           config->bank_a_version, config->bank_a_boot_count, config->bank_a_fail_count);
    secure_boot_debug_print("Bank B Version: %u (Boots: %u, Failures: %u)\n", 
                           config->bank_b_version, config->bank_b_boot_count, config->bank_b_fail_count);
    secure_boot_debug_print("Rollback Counter: %u\n", config->rollback_counter);
}