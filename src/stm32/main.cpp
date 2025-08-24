#include "status_encoder.hpp"
#include "uart_writer.hpp"
#include "port_state.hpp"

int main() {
    UART_HandleTypeDef huart{}; // Assume initialized elsewhere
    UartWriter writer(&huart);
    StatusEncoder encoder;
    std::uint8_t buffer[StatusEncoder::BUFFER_SIZE];

    while (true) {
        for (std::uint32_t port = 0; port < NUM_PORTS; ++port) {
            PortState state = get_port_state(port);
            std::size_t written = 0;
            if (encoder.encode(port, state, buffer, written)) {
                writer.send(buffer, written);
            }
        }
    }
    return 0;
}

