#include "proto/splitter.pb.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main() {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(50051);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    while (true) {
        uint32_t len_n;
        ssize_t n = recv(sock, &len_n, sizeof(len_n), MSG_WAITALL);
        if (n != sizeof(len_n)) break;
        uint32_t len = ntohl(len_n);
        std::vector<char> buf(len);
        n = recv(sock, buf.data(), len, MSG_WAITALL);
        if (n != static_cast<ssize_t>(len)) break;
        splitter::Envelope env;
        if (env.ParseFromArray(buf.data(), len) && env.has_status()) {
            const auto& s = env.status();
            std::cout << "Port " << s.port()
                      << (s.enabled() ? " enabled" : " disabled")
                      << ", frequency: " << s.center_mhz() << " MHz"
                      << ", signal: " << (s.signal_present() ? "present" : "absent")
                      << std::endl;
        }
    }
    close(sock);
    return 0;
}
