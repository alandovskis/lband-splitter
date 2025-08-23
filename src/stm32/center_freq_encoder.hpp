#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

// Encodes the center frequency report into a protobuf envelope
class CenterFreqEncoder {
public:
    static constexpr std::size_t BUFFER_SIZE = 64;

    // Encodes the given center frequency in MHz into the provided buffer.
    // On success returns true and sets out_size to the number of bytes written.
    bool encode(float centerMHz, std::uint8_t* buffer, std::size_t& out_size) const noexcept;
};

