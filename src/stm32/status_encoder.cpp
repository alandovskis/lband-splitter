#include "status_encoder.hpp"

#include "splitter.pb.h"
#include "crc32.hpp"
#include <string>

bool StatusEncoder::encode(std::uint32_t port, const PortState& state,
                           std::uint8_t* buffer, std::size_t& out_size) const noexcept {
    splitter::Envelope env;
    auto* status = env.mutable_status();
    status->set_port(port);
    status->set_enabled(state.enabled);
    status->set_signal_present(state.enabled && state.frequencyMHz > 0.0f);
    status->set_center_mhz(state.frequencyMHz);

    std::string payload;
    status->SerializeToString(&payload);
    env.set_crc32(crc32(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()));

    const auto size = env.ByteSizeLong();
    if (size > BUFFER_SIZE) {
        return false;
    }
    if (!env.SerializeToArray(buffer, static_cast<int>(BUFFER_SIZE))) {
        return false;
    }
    out_size = static_cast<std::size_t>(size);
    return true;
}

