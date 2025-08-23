#pragma once

#include "PortState.hpp"
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <thread>
#include <vector>

// Minimal NETCONF server. Supports enabling/disabling ports via <edit-config>
// and retrieving port state via <get>.
class NetconfServer {
public:
    NetconfServer(std::vector<PortState>& ports, unsigned short port = 8300)
        : ports_(ports),
          acceptor_(ioc_, {boost::asio::ip::tcp::v4(), port}),
          eom_("\x5D\x5D\x3E\x5D\x5D\x3E") {}

    void start() {
        std::thread([this] {
            for (;;) {
                boost::asio::ip::tcp::socket socket(ioc_);
                acceptor_.accept(socket);
                std::thread(&NetconfServer::session, this, std::move(socket))
                    .detach();
            }
        }).detach();
    }

private:
    void session(boost::asio::ip::tcp::socket socket) {
        using boost::asio::buffer;
        using boost::asio::read_until;
        using boost::asio::write;

        const std::string hello =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<hello><capabilities>"
            "<capability>urn:ietf:params:netconf:base:1.0</capability>"
            "</capabilities></hello>" + eom_;
        write(socket, buffer(hello));

        boost::asio::streambuf buf;
        boost::system::error_code ec;
        while (!ec) {
            read_until(socket, buf, eom_, ec);
            if (ec) break;
            std::istream is(&buf);
            std::string rpc((std::istreambuf_iterator<char>(is)), {});
            buf.consume(buf.size());
            auto pos = rpc.find(eom_);
            if (pos != std::string::npos)
                rpc.erase(pos);
            handle_rpc(rpc, socket);
        }
    }

    void handle_rpc(const std::string& rpc,
                    boost::asio::ip::tcp::socket& sock) {
        using boost::property_tree::ptree;
        ptree tree;
        std::stringstream ss(rpc);
        try {
            boost::property_tree::read_xml(ss, tree);
        } catch (...) {
            return;
        }

        ptree reply_tree;
        std::string msg_id =
            tree.get<std::string>("rpc.<xmlattr>.message-id", "0");

        if (tree.get_child_optional("rpc.get")) {
            ptree data;
            ptree ports_node;
            for (size_t i = 0; i < ports_.size(); ++i) {
                ptree port;
                port.put("<xmlattr>.id", i);
                port.put("enabled", ports_[i].enabled.load() ? 1 : 0);
                port.put("frequency", ports_[i].centerFreqMHz.load());
                ports_node.add_child("port", port);
            }
            data.add_child("ports", ports_node);
            ptree reply_rpc;
            reply_rpc.put("<xmlattr>.message-id", msg_id);
            reply_rpc.add_child("data", data);
            reply_tree.add_child("rpc-reply", reply_rpc);
        } else if (auto edit = tree.get_child_optional("rpc.edit-config")) {
            try {
                auto port_node = edit->get_child("port");
                size_t id = port_node.get<size_t>("<xmlattr>.id");
                bool en = port_node.get<bool>("enabled");
                if (id < ports_.size())
                    ports_[id].enabled = en;
                ptree reply_rpc;
                reply_rpc.put("<xmlattr>.message-id", msg_id);
                reply_rpc.put("ok", "");
                reply_tree.add_child("rpc-reply", reply_rpc);
            } catch (...) {
                ptree reply_rpc;
                reply_rpc.put("<xmlattr>.message-id", msg_id);
                reply_rpc.put("rpc-error", "");
                reply_tree.add_child("rpc-reply", reply_rpc);
            }
        } else {
            ptree reply_rpc;
            reply_rpc.put("<xmlattr>.message-id", msg_id);
            reply_rpc.put("rpc-error", "");
            reply_tree.add_child("rpc-reply", reply_rpc);
        }

        std::ostringstream oss;
        boost::property_tree::write_xml(
            oss, reply_tree,
            boost::property_tree::xml_writer_make_settings<std::string>(' ', 0));
        const std::string reply = oss.str() + eom_;
        boost::asio::write(sock, boost::asio::buffer(reply));
    }

    std::vector<PortState>& ports_;
    boost::asio::io_context ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    const std::string eom_;
};

