#include "stm32/status_encoder.hpp"
#include "splitter.pb.h"
#include "crc32.hpp"

int main() {
    StatusEncoder enc;
    PortState st{true, 1.23f};
    std::uint8_t buffer[StatusEncoder::BUFFER_SIZE];
    std::size_t out = 0;
    if (!enc.encode(5, st, buffer, out)) return 1;
    splitter::Envelope env;
    if (!env.ParseFromArray(buffer, static_cast<int>(out))) return 1;
    if (!env.has_status()) return 1;
    const auto& s = env.status();
    if (s.port() != 5 || !s.enabled()) return 1;
    if (s.center_mhz() != 1.23f) return 1;
    std::string payload;
    env.status().SerializeToString(&payload);
    const auto expected = crc32(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size());
    return env.crc32() == expected ? 0 : 1;
}

