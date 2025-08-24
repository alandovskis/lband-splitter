#pragma once

#include <cstddef>
#include <cstdint>
#include "port_state.hpp"

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

// Encodes per-port status messages into a protobuf envelope
class StatusEncoder {
public:
    static constexpr std::size_t BUFFER_SIZE = 64;

    // Encodes the status of a port into the provided buffer.
    // Returns true on success and sets out_size to bytes written.
    bool encode(std::uint32_t port, const PortState& state,
                std::uint8_t* buffer, std::size_t& out_size) const noexcept;
};

