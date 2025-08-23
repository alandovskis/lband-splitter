#include "daemon/netconf_server.hpp"
#include "PortState.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

int main() {
    std::vector<PortState> ports(2);
    ports[1].centerFreqMHz = 123.45;

    NetconfServer server(ports, 8301);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    using boost::asio::ip::tcp;
    boost::asio::io_context ioc;
    tcp::socket sock(ioc);
    sock.connect({tcp::v4(), 8301});

    const std::string eom("\x5D\x5D\x3E\x5D\x5D\x3E");

    boost::asio::streambuf buf;
    boost::asio::read_until(sock, buf, eom);
    buf.consume(buf.size());

    std::string edit =
        "<rpc message-id=\"1\"><edit-config><port id=\"1\"><enabled>true" \
        "</enabled></port></edit-config></rpc>" + eom;
    boost::asio::write(sock, boost::asio::buffer(edit));
    boost::asio::read_until(sock, buf, eom);
    buf.consume(buf.size());
    assert(ports[1].enabled.load());

    std::string get = "<rpc message-id=\"2\"><get/></rpc>" + eom;
    boost::asio::write(sock, boost::asio::buffer(get));
    boost::asio::read_until(sock, buf, eom);
    std::istream is(&buf);
    std::string reply((std::istreambuf_iterator<char>(is)), {});
    buf.consume(buf.size());
    assert(reply.find("port id=\"1\"") != std::string::npos);
    assert(reply.find("<enabled>1</enabled>") != std::string::npos);
    assert(reply.find("<frequency>123.45</frequency>") != std::string::npos);

    return 0;
}

