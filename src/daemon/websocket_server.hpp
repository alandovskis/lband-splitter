#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>

// Simple echo WebSocket server. Launch run() to start listening on the given port.
class WebSocketServer {
public:
    explicit WebSocketServer(unsigned short port)
        : ioc_(1), acceptor_(ioc_, {boost::asio::ip::tcp::v4(), port}) {}

    // Start the server threads. Connections are handled in detached sessions.
    void run() {
        std::thread([this] {
            for (;;) {
                boost::asio::ip::tcp::socket socket(ioc_);
                acceptor_.accept(socket);
                std::thread(&WebSocketServer::session, this, std::move(socket)).detach();
            }
        }).detach();
        std::thread([this] { ioc_.run(); }).detach();
    }

private:
    void session(boost::asio::ip::tcp::socket socket) {
        namespace websocket = boost::beast::websocket;
        websocket::stream<boost::asio::ip::tcp::socket> ws(std::move(socket));
        ws.accept();
        for (;;) {
            boost::beast::flat_buffer buffer;
            ws.read(buffer);
            ws.text(ws.got_text());
            ws.write(buffer.data()); // Echo back
        }
    }

    boost::asio::io_context ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

