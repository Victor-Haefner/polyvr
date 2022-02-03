#include "VRUDPServer.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>
#include <memory>

//#ifdef _WINDOWS // TODO
//#include <ws2tcpip.h>
//#endif

using namespace std;
using namespace OSG;
using namespace boost::asio;
using ip::address;
using ip::udp;

class UDPServer {
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        udp::socket socket;
        thread service;
        //boost::array<char, 1024> recv_buffer;
        //boost::asio::streambuf buffer;
        boost::array<char, 1024> buffer;
        udp::endpoint remote_endpoint;

        function<void (string)> onMessageCb;

        void wait() {
            auto cb = boost::bind(&UDPServer::read_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
            socket.async_receive_from(boost::asio::buffer(buffer), remote_endpoint, cb);
        }

		void run() {
			io_service.run();
		}

        void read_handler(const boost::system::error_code& ec, size_t N) {
            if (ec) { cout << "Receive failed: " << ec.message() << "\n"; return; }
            string msg(buffer.begin(), buffer.begin()+N);
            //cout << "Received: '" << msg << "' (" << ec.message() << ")\n";
            if (onMessageCb) onMessageCb(msg);
            wait();
        }

    public:
        UDPServer() : worker(io_service), socket(io_service) {
            service = thread([this](){ run(); });
        }

        ~UDPServer() { close(); }

        void onMessage( function<void (string)> f ) { onMessageCb = f; }

        void listen(int port) {
            cout << "UDPServer listen on port " << port << endl;
            socket.open(udp::v4());
            //socket.bind(udp::endpoint(address::from_string(IPADDRESS), port));
            socket.bind(udp::endpoint(udp::v4(), port));
            wait();
        }

        void close() {
            io_service.stop();
            socket.cancel();
            boost::system::error_code _error_code;
            socket.shutdown(udp::socket::shutdown_both, _error_code);
            if (service.joinable()) service.join();
        }
};


VRUDPServer::VRUDPServer() { server = new UDPServer(); }
VRUDPServer::~VRUDPServer() { delete server; }

VRUDPServerPtr VRUDPServer::create() { return VRUDPServerPtr(new VRUDPServer()); }

void VRUDPServer::onMessage( function<void(string)> f ) { server->onMessage(f); }
void VRUDPServer::listen(int port) { this->port = port; server->listen(port); }
void VRUDPServer::close() { server->close(); }
int VRUDPServer::getPort() { return port; }
