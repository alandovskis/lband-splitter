#include "stm32/center_freq_encoder.hpp"
#include "splitter.pb.h"
#include "crc32.hpp"
#include <string>

int main() {
    CenterFreqEncoder enc;
    std::uint8_t buffer[CenterFreqEncoder::BUFFER_SIZE];
    std::size_t out = 0;
    if (!enc.encode(1.23f, buffer, out)) return 1;
    splitter::Envelope env;
    if (!env.ParseFromArray(buffer, static_cast<int>(out))) return 1;
    if (!env.has_report()) return 1;
    if (env.report().center_mhz() != 1.23f) return 1;
    std::string payload;
    env.report().SerializeToString(&payload);
    const auto expected = crc32(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size());
    return env.crc32() == expected ? 0 : 1;
}
