#include "VRUDPClient.h"
#include "core/utils/toString.h"
#include "core/utils/VRMutex.h"
#include "core/gui/VRGuiConsole.h"

#include <boost/asio.hpp>
#include <iostream>
#include <list>
#include <thread>

using namespace OSG;
using namespace boost;
using asio::ip::udp;

class UDPClient {
    private:
        asio::io_service io_service;
        asio::io_service::work worker;
        udp::endpoint remote_endpoint;
        udp::socket socket;
        list<string> messages;
        thread service;
        string guard;
        VRMutex mtx;
        bool stop = false;
        bool broken = false;

        vector<asio::ip::udp::endpoint> uriToEndpoints(const string& uri) {
            asio::ip::udp::resolver resolver(io_service);
            asio::ip::udp::resolver::query query(uri, "");
            vector<asio::ip::udp::endpoint> res;
            for(asio::ip::udp::resolver::iterator i = resolver.resolve(query); i != asio::ip::udp::resolver::iterator(); ++i) {
                res.push_back(*i);
            }
            return res;
        }

        void processQueue() {
            if (broken) return;
            VRLock lock(mtx);
            boost::system::error_code ec;
            auto N = socket.send_to(boost::asio::buffer(messages.front()), remote_endpoint, 0, ec);

            if (!ec) {
                VRLock lock(mtx);
                messages.pop_front();
                if (!messages.empty()) processQueue();
            } else {
                cout << " tcp client write ERROR: " << ec.message() << "  N: " << N << ", close socket!" << endl;
                socket.close();
            }
        }

        void runService() { // needed because windows complains..
            io_service.run();
        }

    public:
        UDPClient() : worker(io_service), socket(io_service) {
            socket.open(udp::v4());
            service = thread([this]() { runService(); });
        }

        ~UDPClient() { close(); }

        void close() {
            cout << "close UDP client" << endl;
            stop = true;

            try {
                io_service.stop();
                socket.close();
                system::error_code _error_code;
                socket.shutdown(asio::ip::udp::socket::shutdown_both, _error_code);
            } catch(...) {
                ;
            }

            cout << "join service thread" << endl;
            if (service.joinable()) service.join();
        }

        void connect(string host, int port) {
            cout << "UDPClient::connect " << this << " to: " << host << ", on port " << port << endl;
            try {
                remote_endpoint = udp::endpoint( asio::ip::address::from_string(host), port);
                //socket.connect( udp::endpoint( asio::ip::address::from_string(host), port ));
            } catch(std::exception& e) {
                cout << "UDPClient::connect failed with: " << e.what() << endl;
#ifndef WITHOUT_GTK
                VRConsoleWidget::get("Collaboration")->write( " TCP connect to "+host+":"+toString(port)+" failed with "+e.what()+"\n", "red");
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


VRUDPClient::VRUDPClient() { client = new UDPClient(); }
VRUDPClient::~VRUDPClient() { delete client; }

VRUDPClientPtr VRUDPClient::create() { return VRUDPClientPtr( new VRUDPClient() ); }

void VRUDPClient::connect(string host, int port) { client->connect(host, port); uri = host+":"+toString(port); }
void VRUDPClient::send(const string& message, bool verbose) { client->send(message, verbose); }


