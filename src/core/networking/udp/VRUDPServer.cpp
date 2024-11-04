#include "VRUDPServer.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

#include <cstdlib>
#include <iostream>
#include "core/utils/Thread.h"
#include <string>
#include <memory>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

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
        VRUDPServer* parent = 0;
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        udp::socket socket;
        ::Thread* service = 0;
        //boost::array<char, 1024> recv_buffer;
        //boost::asio::streambuf buffer;
        boost::array<char, 1024> buffer;
        udp::endpoint remote_endpoint;

        function<string (string)> onMessageCb;
        bool deferredMessaging = false;

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

            if (parent) {
                auto& iFlow = parent->getInFlow();
                iFlow.logFlow(N*0.001);
            }

            if (onMessageCb) {
                if (!deferredMessaging) {
                    string res = onMessageCb(msg);
                    if (res != "") {
                        boost::system::error_code ec;
                        auto N = socket.send_to(boost::asio::buffer(res), remote_endpoint, 0, ec);

                        if (parent) {
                            auto& oFlow = parent->getOutFlow();
                            oFlow.logFlow(N*0.001);
                        }
                    }
                } else {
                    auto scene = VRScene::getCurrent();
                    auto cb = VRUpdateCb::create("udpDeferred", [&](){onMessageCb(msg);});
                    scene->queueJob(cb);
                }
            }
            wait();
        }

    public:
        UDPServer(VRUDPServer* s) : parent(s), worker(io_service), socket(io_service) {
            service = new ::Thread("UDPServer_service", [this](){ run(); });
        }

        ~UDPServer() {
            close();
            delete service;
        }

        void onMessage( function<string (string)> f, bool b ) { onMessageCb = f; deferredMessaging = b; }

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
            if (service->joinable()) service->join();
        }
};


VRUDPServer::VRUDPServer(string n) : VRNetworkServer(n) {
    protocol = "udp";
    server = new UDPServer(this);
}

VRUDPServer::~VRUDPServer() { delete server; }

VRUDPServerPtr VRUDPServer::create(string name) {
    auto s = VRUDPServerPtr(new VRUDPServer(name));
    s->regServer(s);
    return s;
}

void VRUDPServer::onMessage( function<string(string)> f, bool b ) { server->onMessage(f, b); }
void VRUDPServer::listen(int port) { this->port = port; server->listen(port); }
void VRUDPServer::close() { server->close(); }
int VRUDPServer::getPort() { return port; }
