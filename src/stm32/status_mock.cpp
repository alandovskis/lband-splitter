#include "PortState.hpp"
#include "crc32.hpp"
#include "proto/splitter.pb.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Simple STM32 mock that toggles port states, prints changes, and streams
// them to a TCP client using protobuf messages.
int main() {
    constexpr std::size_t NUM_PORTS = 4;
    std::vector<PortState> ports(NUM_PORTS);

    // Set up TCP server on port 50051
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(50051);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    listen(server_fd, 1);
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }

    for (;;) {
        for (std::size_t i = 0; i < NUM_PORTS; ++i) {
            bool newEnabled = !ports[i].enabled.load();
            ports[i].enabled = newEnabled;
            ports[i].signalPresent = newEnabled;
            ports[i].centerFreqMHz = newEnabled ? 1000.0 + static_cast<double>(i) : 0.0;
            std::cout << "Port " << i
                      << (newEnabled ? " enabled" : " disabled")
                      << ", frequency: " << ports[i].centerFreqMHz.load() << " MHz"
                      << ", signal: " << (ports[i].signalPresent.load() ? "present" : "absent")
                      << std::endl;

            splitter::Envelope env;
            auto* status = env.mutable_status();
            status->set_port(i);
            status->set_enabled(newEnabled);
            status->set_signal_present(newEnabled);
            status->set_center_mhz(ports[i].centerFreqMHz.load());
            std::string payload = status->SerializeAsString();
            env.set_crc32(crc32(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));
            std::string out = env.SerializeAsString();
            uint32_t len = htonl(static_cast<uint32_t>(out.size()));
            if (send(client_fd, &len, sizeof(len), 0) > 0) {
                send(client_fd, out.data(), out.size(), 0);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    close(client_fd);
    close(server_fd);
    return 0;
}
