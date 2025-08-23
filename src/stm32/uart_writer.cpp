#include "uart_writer.hpp"

void UartWriter::send(const std::uint8_t* data, std::size_t size) const noexcept {
    HAL_UART_Transmit(handle_, const_cast<std::uint8_t*>(data), static_cast<uint16_t>(size), HAL_MAX_DELAY);
}

