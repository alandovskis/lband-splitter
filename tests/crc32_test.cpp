#include "crc32.hpp"
#include <cstring>

int main() {
    const char* msg = "123456789";
    const auto crc = crc32(reinterpret_cast<const std::uint8_t*>(msg), std::strlen(msg));
    return crc == 0xCBF43926u ? 0 : 1;
}
