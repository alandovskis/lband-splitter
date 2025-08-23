#include <iostream>
#include <string>
#include "splitter.pb.h"
#include "crc32.hpp"
#include "websocket_server.hpp"
#include "netconf_server.hpp"
#include "PortState.hpp"
#include <vector>

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::vector<PortState> ports(32);

    // Start WebSocket interface for Angular frontend
    WebSocketServer ws(9002);
    ws.run();

    // NETCONF server for control and monitoring
    NetconfServer netconf(ports);
    netconf.start();

    splitter::Envelope env;
    if (!env.ParseFromIstream(&std::cin)) {
        std::cerr << "No input" << std::endl;
        return 1;
    }

    std::string payload;
    if (env.has_report()) {
        env.report().SerializeToString(&payload);
    } else if (env.has_command()) {
        env.command().SerializeToString(&payload);
    } else {
        std::cerr << "No payload" << std::endl;
        return 1;
    }

    const auto computed = crc32(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size());
    if (computed != env.crc32()) {
        std::cerr << "CRC mismatch" << std::endl;
        return 1;
    }

    if (env.has_report()) {
        std::cout << "Center frequency: " << env.report().center_mhz() << " MHz" << std::endl;
    } else if (env.has_command()) {
        const auto &cmd = env.command();
        if (cmd.type() == splitter::Command::SET_PORT_ENABLED) {
            std::cout << "Port " << cmd.port()
                      << (cmd.enable() ? " enabled" : " disabled") << std::endl;
        } else {
            std::cout << "Received command: "
                      << splitter::Command_Type_Name(cmd.type()) << std::endl;
        }
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
