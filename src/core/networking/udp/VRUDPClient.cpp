#include "VRUDPClient.h"
#include "core/utils/toString.h"
#include "core/utils/VRMutex.h"
#include "core/gui/VRGuiConsole.h"
#include "core/scene/VRSceneManager.h"

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <list>
#include "core/utils/Thread.h"

using namespace OSG;
using boost::asio::ip::udp;

class UDPClient {
    private:
        VRUDPClient* parent = 0;
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        udp::endpoint remote_endpoint;
        udp::socket socket;
        list<string> messages;
        ::Thread service;
        string guard;
        VRMutex mtx;
        bool stop = false;
        bool broken = false;
        function<string (string)> onMessageCb;
        boost::array<char, 32768> buffer;

        vector<boost::asio::ip::udp::endpoint> uriToEndpoints(const string& uri) {
            boost::asio::ip::udp::resolver resolver(io_service);
            boost::asio::ip::udp::resolver::query query(uri, "");
            vector<boost::asio::ip::udp::endpoint> res;
            for(boost::asio::ip::udp::resolver::iterator i = resolver.resolve(query); i != boost::asio::ip::udp::resolver::iterator(); ++i) {
                res.push_back(*i);
            }
            return res;
        }

        bool read_handler(const boost::system::error_code& ec, size_t N) {
            if (parent) {
                auto& iFlow = parent->getInFlow();
                iFlow.logFlow(N*0.001);
            }

            if (ec) { cout << "UDPClient receive failed: " << ec.message() << "\n"; broken = true; return false; }
            string msg(buffer.begin(), buffer.begin()+N);
            if (N == buffer.size()) {
                cout << "Warning in UDPClient::read_handler: message reached buffer size, data loss probable!" << endl;
            }
            //cout << "Received: " << N << "\n";
            //cout << "Received: " << N << " (" << ec.message() << ")\n";
            if (onMessageCb) {
                string res = onMessageCb(msg);
                if (res != "") {
                    boost::system::error_code ec;
                    auto N = socket.send_to(boost::asio::buffer(res), remote_endpoint, 0, ec);
                    // TODO: respond to client
                }
            }

            return true;
        }

        bool read() {
#ifdef _WIN32
			return true; // doesnt work under windows..
#endif
            if (broken) return false;

            auto onRead = [this](const boost::system::error_code& ec, size_t N) {
                read_handler(ec, N);
                read();
            };

            //auto cb = boost::bind(&UDPClient::read_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
            socket.async_receive_from(boost::asio::buffer(buffer), remote_endpoint, onRead);
            return true;
        }

        void processQueue() {
            if (broken) return;
            VRLock lock(mtx);
            boost::system::error_code ec;
            auto N = socket.send_to(boost::asio::buffer(messages.front()), remote_endpoint, 0, ec);

            if (parent) {
                auto& oFlow = parent->getOutFlow();
                oFlow.logFlow(N*0.001);
            }

            if (!ec) {
                VRLock lock(mtx);
                messages.pop_front();
                if (!messages.empty()) processQueue();
            } else {
                cout << " udp client write ERROR: " << ec.message() << "  N: " << N << ", close socket!" << endl;
                socket.close();
            }
        }

        void runService() { // needed because windows complains..
            io_service.run();
        }

    public:
        UDPClient(VRUDPClient* c) : parent(c), worker(io_service), socket(io_service) {
            socket.open(udp::v4());
            service = ::Thread("UDPClient_service", [this]() { runService(); });
        }

        ~UDPClient() { close(); }

        void onMessage( function<string (string)> f ) { onMessageCb = f; }

        void close() {
            cout << "close UDP client" << endl;
            stop = true;

            try {
                io_service.stop();
                socket.close();
                boost::system::error_code _error_code;
                socket.shutdown(boost::asio::ip::udp::socket::shutdown_both, _error_code);
            } catch(...) {
                ;
            }

            cout << "join service thread" << endl;
            if (service.joinable()) service.join();
        }

        void connect(string host, int port) {
            cout << "UDPClient::connect " << this << " to: " << host << ", on port " << port << endl;
            try {
                remote_endpoint = udp::endpoint( boost::asio::ip::address::from_string(host), port);
                //socket.connect( udp::endpoint( boost::asio::ip::address::from_string(host), port ));
                read();
            } catch(std::exception& e) {
                cout << "UDPClient::connect failed with: " << e.what() << endl;
#ifndef WITHOUT_IMGUI
                VRConsoleWidget::get("Collaboration")->write( " UDP connect to "+host+":"+toString(port)+" failed with "+e.what()+"\n", "red");
#endif
            }
        }

        void send(string msg, bool verbose) {
            if (broken) return;
            VRLock lock(mtx);
            this->guard = guard;
            msg += guard;
            size_t S = msg.size();
            double s = S/1000.0;
            //if (verbose) cout << "UDPClient::send " << this << " msg: " << msg << ", " << s << " kb" << endl;
            if (verbose) cout << "UDPClient::send " << this << ", " << s << " kb" << endl;
            bool write_in_progress = !messages.empty();
            messages.push_back(msg);
            if (!write_in_progress) processQueue();
        }

        bool connected() {
            if (broken) return false;
            return socket.is_open();
        }
};


VRUDPClient::VRUDPClient(string name) : VRNetworkClient(name) {
    protocol = "udp";
    client = new UDPClient(this);
}

VRUDPClient::~VRUDPClient() {
    delete client;
}

VRUDPClientPtr VRUDPClient::create(string name) {
    auto c = VRUDPClientPtr(new VRUDPClient(name));
    VRSceneManager::get()->regNetworkClient(c);
    return c;
}

void VRUDPClient::onMessage( function<string(string)> f ) { client->onMessage(f); }
void VRUDPClient::connect(string host, int port) { client->connect(host, port); uri = host+":"+toString(port); }
bool VRUDPClient::isConnected(string host, int port) { return bool(host+":"+toString(port) == uri); }
void VRUDPClient::send(const string& message, string guard, bool verbose) { client->send(message+guard, verbose); }
