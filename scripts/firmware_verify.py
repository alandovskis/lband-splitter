#!/usr/bin/env python3
"""
Firmware verification utility for STM32F4 secure boot system.

This script verifies signed firmware binaries and validates their
headers, signatures, and integrity checksums.
"""

import os
import sys
import struct
import hashlib
import argparse
from pathlib import Path

try:
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import ed25519
    from cryptography.hazmat.primitives import serialization
    from cryptography.exceptions import InvalidSignature
except ImportError:
    print("Error: cryptography library not found.")
    print("Install with: pip install cryptography")
    sys.exit(1)

# Constants from secure_boot.h
SECURE_BOOT_MAGIC = 0x53454342  # "SECB" in little endian
SECURE_BOOT_KEY_SIZE = 32       # 256-bit keys
SECURE_BOOT_SIGNATURE_SIZE = 64 # 512-bit signatures (Ed25519)
SECURE_BOOT_HASH_SIZE = 32      # SHA-256 hash size

class CRC32:
    """CRC32 implementation matching the bootloader"""
    
    def __init__(self):
        # CRC32 lookup table (same as in secure_boot.c)
        self.table = [
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
        ]
    
    def calculate(self, data):
        """Calculate CRC32 of data"""
        crc = 0xFFFFFFFF
        for byte in data:
            crc = self.table[(crc ^ byte) & 0xFF] ^ (crc >> 8)
        return crc ^ 0xFFFFFFFF

class SecureFirmwareHeader:
    """Secure firmware header structure"""
    
    # Header structure format (little endian)
    STRUCT_FORMAT = '<LLLL32s64sLL16sL'
    STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)
    
    def __init__(self):
        self.magic = 0
        self.version = 0
        self.size = 0
        self.crc32 = 0
        self.sha256_hash = b''
        self.signature = b''
        self.timestamp = 0
        self.rollback_counter = 0
        self.reserved = b''
        self.header_crc = 0
    
    @classmethod
    def from_bytes(cls, data):
        """Create header from bytes"""
        if len(data) < cls.STRUCT_SIZE:
            raise ValueError(f"Data too short for header: {len(data)} < {cls.STRUCT_SIZE}")
        
        header = cls()
        fields = struct.unpack(cls.STRUCT_FORMAT, data[:cls.STRUCT_SIZE])
        
        header.magic = fields[0]
        header.version = fields[1]
        header.size = fields[2]
        header.crc32 = fields[3]
        header.sha256_hash = fields[4]
        header.signature = fields[5]
        header.timestamp = fields[6]
        header.rollback_counter = fields[7]
        header.reserved = fields[8]
        header.header_crc = fields[9]
        
        return header
    
    def pack(self):
        """Pack header into bytes"""
        return struct.pack(self.STRUCT_FORMAT,
                          self.magic,
                          self.version,
                          self.size,
                          self.crc32,
                          self.sha256_hash,
                          self.signature,
                          self.timestamp,
                          self.rollback_counter,
                          self.reserved,
                          self.header_crc)
    
    def calculate_header_crc(self):
        """Calculate CRC of header (excluding header_crc field)"""
        temp_crc = self.header_crc
        self.header_crc = 0
        header_data = self.pack()[:-4]  # Exclude header_crc field
        self.header_crc = temp_crc
        return CRC32().calculate(header_data)
    
    def print_info(self):
        """Print header information"""
        print("=== Firmware Header Information ===")
        print(f"Magic: 0x{self.magic:08X} ({'VALID' if self.magic == SECURE_BOOT_MAGIC else 'INVALID'})")
        print(f"Version: {self.version}")
        print(f"Size: {self.size} bytes")
        print(f"CRC32: 0x{self.crc32:08X}")
        print(f"SHA-256: {self.sha256_hash.hex()}")
        print(f"Signature: {self.signature.hex()}")
        print(f"Timestamp: {self.timestamp}")
        print(f"Rollback Counter: {self.rollback_counter}")
        print(f"Header CRC: 0x{self.header_crc:08X}")

class FirmwareVerifier:
    """Firmware verification utility"""
    
    def __init__(self, public_key_path=None):
        self.crc32 = CRC32()
        self.public_key = None
        
        if public_key_path:
            self.load_public_key(public_key_path)
    
    def load_public_key(self, public_key_path):
        """Load public key from file"""
        try:
            # Try to load as PEM first
            if public_key_path.endswith('.pem'):
                with open(public_key_path, 'rb') as f:
                    public_pem = f.read()
                
                self.public_key = serialization.load_pem_public_key(public_pem)
                
            # Try to load as raw binary
            elif public_key_path.endswith('.bin'):
                with open(public_key_path, 'rb') as f:
                    public_raw = f.read()
                
                if len(public_raw) != SECURE_BOOT_KEY_SIZE:
                    raise ValueError(f"Invalid public key size: {len(public_raw)} != {SECURE_BOOT_KEY_SIZE}")
                
                self.public_key = ed25519.Ed25519PublicKey.from_public_bytes(public_raw)
            
            else:
                raise ValueError("Unsupported public key format (use .pem or .bin)")
            
            print(f"Loaded public key from: {public_key_path}")
            
        except Exception as e:
            print(f"Error loading public key: {e}")
            sys.exit(1)
    
    def verify_firmware(self, firmware_path, check_signature=True):
        """Verify signed firmware binary"""
        print(f"Verifying firmware: {firmware_path}")
        
        # Read firmware file
        try:
            with open(firmware_path, 'rb') as f:
                firmware_data = f.read()
        except Exception as e:
            print(f"Error reading firmware: {e}")
            return False
        
        if len(firmware_data) < SecureFirmwareHeader.STRUCT_SIZE:
            print(f"Error: Firmware too small: {len(firmware_data)} bytes")
            return False
        
        # Parse header
        try:
            header = SecureFirmwareHeader.from_bytes(firmware_data)
            header.print_info()
        except Exception as e:
            print(f"Error parsing header: {e}")
            return False
        
        # Verify magic number
        if header.magic != SECURE_BOOT_MAGIC:
            print(f"❌ Invalid magic number: 0x{header.magic:08X}")
            return False
        print("✅ Magic number valid")
        
        # Verify header CRC
        calculated_header_crc = header.calculate_header_crc()
        if calculated_header_crc != header.header_crc:
            print(f"❌ Header CRC mismatch: calculated=0x{calculated_header_crc:08X}, expected=0x{header.header_crc:08X}")
            return False
        print("✅ Header CRC valid")
        
        # Verify firmware size
        if len(firmware_data) != header.size:
            print(f"❌ Size mismatch: file={len(firmware_data)}, header={header.size}")
            return False
        print("✅ Firmware size valid")
        
        # Extract firmware data (after header)
        firmware_payload = firmware_data[SecureFirmwareHeader.STRUCT_SIZE:]
        
        # Verify firmware CRC32
        calculated_crc32 = self.crc32.calculate(firmware_payload)
        if calculated_crc32 != header.crc32:
            print(f"❌ Firmware CRC32 mismatch: calculated=0x{calculated_crc32:08X}, expected=0x{header.crc32:08X}")
            return False
        print("✅ Firmware CRC32 valid")
        
        # Verify SHA-256 hash
        sha256 = hashlib.sha256()
        sha256.update(firmware_payload)
        calculated_hash = sha256.digest()
        if calculated_hash != header.sha256_hash:
            print(f"❌ SHA-256 hash mismatch:")
            print(f"  Calculated: {calculated_hash.hex()}")
            print(f"  Expected:   {header.sha256_hash.hex()}")
            return False
        print("✅ SHA-256 hash valid")
        
        # Verify signature if requested and public key available
        if check_signature:
            if not self.public_key:
                print("⚠️  No public key provided, skipping signature verification")
            else:
                try:
                    # Create header data for signing (excluding signature field)
                    header_for_signing = struct.pack('<LLLL32s',
                                                    header.magic,
                                                    header.version,
                                                    header.size,
                                                    header.crc32,
                                                    header.sha256_hash)
                    
                    self.public_key.verify(header.signature, header_for_signing)
                    print("✅ Ed25519 signature valid")
                    
                except InvalidSignature:
                    print("❌ Ed25519 signature verification failed")
                    return False
                except Exception as e:
                    print(f"❌ Error verifying signature: {e}")
                    return False
        
        print("\n🎉 Firmware verification successful!")
        return True
    
    def extract_firmware(self, signed_firmware_path, output_path):
        """Extract raw firmware from signed firmware file"""
        print(f"Extracting firmware from: {signed_firmware_path}")
        
        # Read signed firmware
        try:
            with open(signed_firmware_path, 'rb') as f:
                signed_data = f.read()
        except Exception as e:
            print(f"Error reading signed firmware: {e}")
            return False
        
        if len(signed_data) < SecureFirmwareHeader.STRUCT_SIZE:
            print("Error: File too small to contain header")
            return False
        
        # Extract firmware data (after header)
        firmware_data = signed_data[SecureFirmwareHeader.STRUCT_SIZE:]
        
        # Write extracted firmware
        try:
            with open(output_path, 'wb') as f:
                f.write(firmware_data)
            
            print(f"Extracted firmware saved to: {output_path}")
            print(f"Extracted size: {len(firmware_data)} bytes")
            return True
            
        except Exception as e:
            print(f"Error writing extracted firmware: {e}")
            return False

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Firmware verification utility for STM32F4 secure boot')
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Verify firmware command
    verify_parser = subparsers.add_parser('verify', help='Verify signed firmware')
    verify_parser.add_argument('--firmware', required=True, help='Signed firmware file')
    verify_parser.add_argument('--public-key', help='Public key file (PEM or binary)')
    verify_parser.add_argument('--no-signature', action='store_true', help='Skip signature verification')
    
    # Extract firmware command
    extract_parser = subparsers.add_parser('extract', help='Extract raw firmware from signed file')
    extract_parser.add_argument('--signed-firmware', required=True, help='Signed firmware file')
    extract_parser.add_argument('--output', required=True, help='Output raw firmware file')
    
    # Info command
    info_parser = subparsers.add_parser('info', help='Show firmware header information')
    info_parser.add_argument('--firmware', required=True, help='Signed firmware file')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    if args.command == 'verify':
        if not os.path.exists(args.firmware):
            print(f"Error: Firmware file not found: {args.firmware}")
            sys.exit(1)
        
        public_key = None
        if args.public_key:
            if not os.path.exists(args.public_key):
                print(f"Error: Public key file not found: {args.public_key}")
                sys.exit(1)
            public_key = args.public_key
        
        verifier = FirmwareVerifier(public_key)
        success = verifier.verify_firmware(args.firmware, not args.no_signature)
        sys.exit(0 if success else 1)
        
    elif args.command == 'extract':
        if not os.path.exists(args.signed_firmware):
            print(f"Error: Signed firmware file not found: {args.signed_firmware}")
            sys.exit(1)
        
        verifier = FirmwareVerifier()
        success = verifier.extract_firmware(args.signed_firmware, args.output)
        sys.exit(0 if success else 1)
        
    elif args.command == 'info':
        if not os.path.exists(args.firmware):
            print(f"Error: Firmware file not found: {args.firmware}")
            sys.exit(1)
        
        try:
            with open(args.firmware, 'rb') as f:
                firmware_data = f.read()
            
            if len(firmware_data) < SecureFirmwareHeader.STRUCT_SIZE:
                print("Error: File too small to contain header")
                sys.exit(1)
            
            header = SecureFirmwareHeader.from_bytes(firmware_data)
            header.print_info()
            
        except Exception as e:
            print(f"Error reading firmware info: {e}")
            sys.exit(1)

if __name__ == '__main__':
    main()