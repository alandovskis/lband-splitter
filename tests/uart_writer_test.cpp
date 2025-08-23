#include "stm32/uart_writer.hpp"

static uint16_t last_size = 0;
extern "C" void HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t*, uint16_t size, uint32_t) {
    last_size = size;
}

int main() {
    UART_HandleTypeDef handle{};
    UartWriter writer(&handle);
    const std::uint8_t data[5] = {1,2,3,4,5};
    writer.send(data, 5);
    return last_size == 5 ? 0 : 1;
}
