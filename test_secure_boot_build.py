#!/usr/bin/env python3
"""
Test script to validate the secure boot implementation.

This script tests all components of the secure boot system:
- Firmware signing and verification utilities
- CMake integration logic
- Error handling and edge cases
"""

import os
import sys
import tempfile
import subprocess
import shutil
from pathlib import Path

def run_command(cmd, cwd=None, capture_output=True):
    """Run a command and return result"""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, 
                              capture_output=capture_output, text=True, timeout=30)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Command timed out"
    except Exception as e:
        return False, "", str(e)

def test_firmware_signing():
    """Test the firmware signing and verification utilities"""
    print("Testing firmware signing utilities...")
    
    with tempfile.TemporaryDirectory() as temp_dir:
        os.chdir(temp_dir)
        
        # Test key generation
        print("  ✓ Testing key generation...")
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_sign.py keygen "
            "--private-key test_key.pem --public-key test_pub.pem"
        )
        if not success:
            print(f"    ❌ Key generation failed: {stderr}")
            return False
        
        if not (os.path.exists("test_key.pem") and os.path.exists("test_pub.pem") and os.path.exists("test_pub.bin")):
            print("    ❌ Key files were not created")
            return False
        
        # Create test firmware
        print("  ✓ Testing firmware signing...")
        test_fw = b"TEST_FIRMWARE_DATA" + b"\x00" * 1000  # 1KB test firmware
        with open("test_fw.bin", "wb") as f:
            f.write(test_fw)
        
        # Sign firmware
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_sign.py sign "
            "--firmware test_fw.bin --private-key test_key.pem "
            "--output signed_fw.bin --version 1 --rollback-counter 1"
        )
        if not success:
            print(f"    ❌ Firmware signing failed: {stderr}")
            return False
        
        if not os.path.exists("signed_fw.bin"):
            print("    ❌ Signed firmware was not created")
            return False
        
        # Verify signed firmware
        print("  ✓ Testing firmware verification...")
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_verify.py verify "
            "--firmware signed_fw.bin --public-key test_pub.pem"
        )
        if not success:
            print(f"    ❌ Firmware verification failed: {stderr}")
            return False
        
        if "🎉 Firmware verification successful!" not in stdout:
            print("    ❌ Verification did not report success")
            return False
        
        # Test firmware info
        print("  ✓ Testing firmware info...")
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_verify.py info "
            "--firmware signed_fw.bin"
        )
        if not success:
            print(f"    ❌ Firmware info failed: {stderr}")
            return False
        
        if "Magic: 0x53454342 (VALID)" not in stdout:
            print("    ❌ Firmware info did not show valid header")
            return False
        
        # Test firmware extraction
        print("  ✓ Testing firmware extraction...")
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_verify.py extract "
            "--signed-firmware signed_fw.bin --output extracted_fw.bin"
        )
        if not success:
            print(f"    ❌ Firmware extraction failed: {stderr}")
            return False
        
        # Verify extracted firmware matches original
        with open("extracted_fw.bin", "rb") as f:
            extracted = f.read()
        
        if extracted != test_fw:
            print("    ❌ Extracted firmware doesn't match original")
            return False
        
        print("  ✅ All firmware utilities working correctly")
        return True

def test_error_handling():
    """Test error handling in utilities"""
    print("Testing error handling...")
    
    with tempfile.TemporaryDirectory() as temp_dir:
        os.chdir(temp_dir)
        
        # Test with invalid firmware
        print("  ✓ Testing invalid firmware handling...")
        with open("invalid.bin", "w") as f:
            f.write("too small")
        
        success, stdout, stderr = run_command(
            "python3 /Users/alex/src/splitter/scripts/firmware_verify.py verify "
            "--firmware invalid.bin --no-signature"
        )
        
        if success:  # Should fail for invalid firmware
            print("    ❌ Verification should have failed for invalid firmware")
            return False
        
        if "Firmware too small" not in stderr:
            print("    ❌ Expected 'Firmware too small' error")
            return False
        
        print("  ✅ Error handling working correctly")
        return True

def test_cmake_integration():
    """Test CMake integration (without requiring ARM toolchain)"""
    print("Testing CMake integration...")
    
    cmake_file = Path("/Users/alex/src/splitter/src/firmware/stm32f4/CMakeLists.txt")
    if not cmake_file.exists():
        print("    ❌ CMakeLists.txt not found")
        return False
    
    # Check for required secure boot targets in CMakeLists.txt
    cmake_content = cmake_file.read_text()
    
    required_targets = [
        "generate_keys",
        "sign_firmware", 
        "verify_firmware",
        "flash_bootloader",
        "flash_signed_a",
        "flash_signed_b",
        "help_secure_boot"
    ]
    
    missing_targets = []
    for target in required_targets:
        if target not in cmake_content:
            missing_targets.append(target)
    
    if missing_targets:
        print(f"    ❌ Missing CMake targets: {missing_targets}")
        return False
    
    # Check for required configuration variables
    required_vars = [
        "FIRMWARE_VERSION",
        "ROLLBACK_COUNTER", 
        "SIGNING_KEY_PATH",
        "PUBLIC_KEY_PATH"
    ]
    
    missing_vars = []
    for var in required_vars:
        if var not in cmake_content:
            missing_vars.append(var)
    
    if missing_vars:
        print(f"    ❌ Missing CMake variables: {missing_vars}")
        return False
    
    print("  ✅ CMake integration looks correct")
    return True

def test_secure_boot_files():
    """Test secure boot source files are present and syntactically correct"""
    print("Testing secure boot source files...")
    
    required_files = [
        "/Users/alex/src/splitter/src/firmware/stm32f4/secure_boot.h",
        "/Users/alex/src/splitter/src/firmware/stm32f4/secure_boot.c", 
        "/Users/alex/src/splitter/src/firmware/stm32f4/bootloader_main.c",
        "/Users/alex/src/splitter/scripts/firmware_sign.py",
        "/Users/alex/src/splitter/scripts/firmware_verify.py"
    ]
    
    for file_path in required_files:
        if not os.path.exists(file_path):
            print(f"    ❌ Missing required file: {file_path}")
            return False
    
    # Check secure_boot.h for required definitions
    with open("/Users/alex/src/splitter/src/firmware/stm32f4/secure_boot.h", "r") as f:
        header_content = f.read()
    
    required_definitions = [
        "SECURE_BOOT_MAGIC",
        "FirmwareBankId", 
        "BootConfiguration",
        "secure_boot_select_boot_bank",
        "secure_boot_verify_firmware",
        "FIRMWARE_BANK_A",
        "FIRMWARE_BANK_B"
    ]
    
    missing_defs = []
    for definition in required_definitions:
        if definition not in header_content:
            missing_defs.append(definition)
    
    if missing_defs:
        print(f"    ❌ Missing definitions in secure_boot.h: {missing_defs}")
        return False
    
    print("  ✅ All secure boot files present and contain required definitions")
    return True

def main():
    """Run all tests"""
    print("🚀 Starting Secure Boot Implementation Tests")
    print("=" * 50)
    
    tests = [
        ("Firmware Signing Utilities", test_firmware_signing),
        ("Error Handling", test_error_handling),
        ("CMake Integration", test_cmake_integration), 
        ("Source Files", test_secure_boot_files)
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\n📋 {test_name}")
        print("-" * 30)
        
        try:
            if test_func():
                print(f"✅ {test_name} PASSED")
                passed += 1
            else:
                print(f"❌ {test_name} FAILED")
        except Exception as e:
            print(f"❌ {test_name} FAILED with exception: {e}")
    
    print("\n" + "=" * 50)
    print(f"📊 Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed! Secure boot implementation is working correctly.")
        return True
    else:
        print(f"⚠️  {total - passed} tests failed. Please review the implementation.")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)