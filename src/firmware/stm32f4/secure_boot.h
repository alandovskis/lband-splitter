#ifndef SECURE_BOOT_H
#define SECURE_BOOT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Secure boot configuration
#define SECURE_BOOT_VERSION         1
#define SECURE_BOOT_MAGIC          0x53454342  // "SECB" in little endian
#define SECURE_BOOT_KEY_SIZE       32          // 256-bit keys
#define SECURE_BOOT_SIGNATURE_SIZE 64          // 512-bit signatures (Ed25519)
#define SECURE_BOOT_HASH_SIZE      32          // SHA-256 hash size

// Memory layout definitions for A/B firmware banks
#define BOOTLOADER_START_ADDR      0x08000000  // Bootloader at start of flash
#define BOOTLOADER_SIZE            0x8000      // 32KB bootloader
#define FIRMWARE_BANK_SIZE         ((1024*1024 - BOOTLOADER_SIZE) / 2)  // ~496KB per bank

// A/B firmware bank addresses
#define FIRMWARE_BANK_A_ADDR       (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE)
#define FIRMWARE_BANK_B_ADDR       (FIRMWARE_BANK_A_ADDR + FIRMWARE_BANK_SIZE)

// Legacy compatibility
#define APPLICATION_START_ADDR     FIRMWARE_BANK_A_ADDR
#define APPLICATION_MAX_SIZE       FIRMWARE_BANK_SIZE

// Public key storage (embedded in bootloader)
#define PUBLIC_KEY_FLASH_ADDR      (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE - 0x100)

// Boot configuration storage (in last sector before app)
#define BOOT_CONFIG_FLASH_ADDR     (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE - 0x200)

// Secure boot states
typedef enum {
    SECURE_BOOT_STATE_UNINITIALIZED = 0,
    SECURE_BOOT_STATE_VERIFYING,
    SECURE_BOOT_STATE_VERIFIED,
    SECURE_BOOT_STATE_FAILED,
    SECURE_BOOT_STATE_ROLLBACK
} SecureBootState;

// Boot result codes
typedef enum {
    SECURE_BOOT_OK = 0,
    SECURE_BOOT_ERR_INVALID_HEADER,
    SECURE_BOOT_ERR_INVALID_SIGNATURE,
    SECURE_BOOT_ERR_HASH_MISMATCH,
    SECURE_BOOT_ERR_INVALID_VERSION,
    SECURE_BOOT_ERR_FLASH_ERROR,
    SECURE_BOOT_ERR_CRYPTO_ERROR,
    SECURE_BOOT_ERR_ROLLBACK_PROTECTION
} SecureBootResult;

// Firmware header structure (must be at start of application)
typedef struct __attribute__((packed)) {
    uint32_t magic;                                    // SECURE_BOOT_MAGIC
    uint32_t version;                                  // Firmware version
    uint32_t size;                                     // Firmware size in bytes
    uint32_t crc32;                                    // CRC32 of firmware data
    uint8_t sha256_hash[SECURE_BOOT_HASH_SIZE];       // SHA-256 hash of firmware
    uint8_t signature[SECURE_BOOT_SIGNATURE_SIZE];    // Ed25519 signature
    uint32_t timestamp;                                // Build timestamp
    uint32_t rollback_counter;                         // Anti-rollback counter
    uint8_t reserved[16];                              // Reserved for future use
    uint32_t header_crc;                               // CRC of this header
} SecureFirmwareHeader;

// Public key structure
typedef struct __attribute__((packed)) {
    uint8_t key_data[SECURE_BOOT_KEY_SIZE];           // Ed25519 public key
    uint32_t key_version;                              // Key version
    uint32_t key_flags;                                // Key usage flags
    uint8_t key_id[8];                                 // Key identifier
    uint32_t key_crc;                                  // CRC of key data
} SecurePublicKey;

// Firmware bank identifiers
typedef enum {
    FIRMWARE_BANK_A = 0,
    FIRMWARE_BANK_B = 1,
    FIRMWARE_BANK_INVALID = 0xFF
} FirmwareBankId;

// Boot configuration structure (stored in flash)
typedef struct __attribute__((packed)) {
    uint32_t magic;                    // Boot config magic
    FirmwareBankId active_bank;        // Currently active bank
    FirmwareBankId pending_bank;       // Pending update bank
    uint32_t bank_a_version;           // Bank A firmware version
    uint32_t bank_b_version;           // Bank B firmware version
    uint32_t bank_a_boot_count;        // Bank A successful boots
    uint32_t bank_b_boot_count;        // Bank B successful boots
    uint32_t bank_a_fail_count;        // Bank A consecutive failures
    uint32_t bank_b_fail_count;        // Bank B consecutive failures
    bool update_in_progress;           // Atomic update flag
    uint32_t rollback_counter;         // Global rollback counter
    uint8_t reserved[32];              // Reserved for future use
    uint32_t config_crc;               // CRC of this structure
} BootConfiguration;

// Boot context structure
typedef struct {
    SecureBootState state;
    SecureBootResult last_result;
    SecureFirmwareHeader firmware_header;
    SecurePublicKey public_key;
    BootConfiguration boot_config;
    uint32_t boot_count;
    uint32_t failed_boots;
    bool development_mode;
    bool debug_enabled;
} SecureBootContext;

// Core secure boot functions
SecureBootResult secure_boot_init(void);
SecureBootResult secure_boot_verify_firmware(uint32_t firmware_addr);
SecureBootResult secure_boot_jump_to_application(uint32_t app_addr);
void secure_boot_emergency_recovery(void);

// Cryptographic functions
SecureBootResult secure_boot_verify_signature(const uint8_t* data, uint32_t data_len,
                                             const uint8_t* signature,
                                             const SecurePublicKey* public_key);
SecureBootResult secure_boot_calculate_hash(const uint8_t* data, uint32_t data_len,
                                           uint8_t* hash_output);
uint32_t secure_boot_calculate_crc32(const uint8_t* data, uint32_t length);

// Flash operations
SecureBootResult secure_boot_read_flash(uint32_t address, uint8_t* buffer, uint32_t length);
SecureBootResult secure_boot_write_flash(uint32_t address, const uint8_t* data, uint32_t length);
SecureBootResult secure_boot_erase_flash_sector(uint32_t sector_addr);

// Public key management
SecureBootResult secure_boot_load_public_key(SecurePublicKey* key);
SecureBootResult secure_boot_validate_public_key(const SecurePublicKey* key);

// Version and rollback protection
SecureBootResult secure_boot_check_version(uint32_t firmware_version);
SecureBootResult secure_boot_update_rollback_counter(uint32_t new_counter);
uint32_t secure_boot_get_rollback_counter(void);

// Debug and development support
void secure_boot_set_development_mode(bool enabled);
bool secure_boot_is_development_mode(void);
void secure_boot_enable_debug_output(bool enabled);
void secure_boot_debug_print(const char* format, ...);

// Boot statistics and monitoring
SecureBootContext* secure_boot_get_context(void);
void secure_boot_increment_boot_counter(void);
void secure_boot_record_boot_failure(void);
bool secure_boot_is_recovery_needed(void);

// Hardware security features
void secure_boot_enable_flash_protection(void);
void secure_boot_disable_jtag_debug(void);
void secure_boot_enable_readout_protection(void);
void secure_boot_configure_mpu(void);

// A/B firmware bank management
SecureBootResult secure_boot_load_boot_config(BootConfiguration* config);
SecureBootResult secure_boot_save_boot_config(const BootConfiguration* config);
SecureBootResult secure_boot_select_boot_bank(FirmwareBankId* selected_bank);
uint32_t secure_boot_get_bank_address(FirmwareBankId bank);
SecureBootResult secure_boot_verify_both_banks(FirmwareBankId* primary, FirmwareBankId* fallback);

// Firmware update operations
SecureBootResult secure_boot_begin_update(FirmwareBankId target_bank);
SecureBootResult secure_boot_commit_update(FirmwareBankId updated_bank);
SecureBootResult secure_boot_rollback_update(void);
bool secure_boot_is_update_pending(void);

// Bank status and management
bool secure_boot_is_bank_valid(FirmwareBankId bank);
uint32_t secure_boot_get_bank_version(FirmwareBankId bank);
uint32_t secure_boot_get_bank_boot_count(FirmwareBankId bank);
SecureBootResult secure_boot_mark_bank_successful(FirmwareBankId bank);
SecureBootResult secure_boot_mark_bank_failed(FirmwareBankId bank);

// Utility functions
const char* secure_boot_result_to_string(SecureBootResult result);
const char* secure_boot_state_to_string(SecureBootState state);
const char* secure_boot_bank_to_string(FirmwareBankId bank);
void secure_boot_print_firmware_info(const SecureFirmwareHeader* header);
void secure_boot_print_boot_config(const BootConfiguration* config);

// Constants for configuration
#define SECURE_BOOT_MAX_FAILED_BOOTS    3
#define SECURE_BOOT_RECOVERY_TIMEOUT_MS 5000
#define SECURE_BOOT_WATCHDOG_TIMEOUT_MS 10000
#define BOOT_CONFIG_MAGIC              0x424F4F54  // "BOOT" in little endian

// Key usage flags
#define SECURE_BOOT_KEY_FLAG_FIRMWARE_SIGN  (1 << 0)
#define SECURE_BOOT_KEY_FLAG_BOOTLOADER     (1 << 1)
#define SECURE_BOOT_KEY_FLAG_DEVELOPMENT    (1 << 2)
#define SECURE_BOOT_KEY_FLAG_BACKUP         (1 << 3)

// Build-time configuration
#ifndef SECURE_BOOT_ENABLE_DEBUG
#define SECURE_BOOT_ENABLE_DEBUG 0
#endif

#ifndef SECURE_BOOT_ENABLE_DEVELOPMENT_MODE
#define SECURE_BOOT_ENABLE_DEVELOPMENT_MODE 0
#endif

#ifndef SECURE_BOOT_ENABLE_ROLLBACK_PROTECTION
#define SECURE_BOOT_ENABLE_ROLLBACK_PROTECTION 1
#endif

#ifdef __cplusplus
}
#endif

#endif // SECURE_BOOT_H