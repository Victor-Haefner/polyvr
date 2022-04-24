#include "VRTCPServer.h"
#include "VRTCPUtils.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

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
using ip::tcp;


class Session {
    public:
        tcp::socket socket;
        string guard;
        boost::asio::streambuf buffer;
        function<string (string)> onMessageCb;
        //enum { max_length = 1024 };
        //char data_[max_length];

    public:
        Session(boost::asio::io_service& io, string g, function<string (string)> cb)
            : socket(io), guard(g), onMessageCb(cb) {
            ;
        }

        ~Session() {
            socket.cancel();
            boost::system::error_code _error_code;
            socket.shutdown(tcp::socket::shutdown_both, _error_code);
        }

        void start() {
            /*socket.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&Session::handle_read, this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));*/

            if (guard == "") boost::asio::async_read( socket, buffer, boost::asio::transfer_at_least(1), bind(&Session::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
            else boost::asio::async_read_until( socket, buffer, guard, bind(&Session::read_handler, this, std::placeholders::_1, std::placeholders::_2) );

        }

        void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) start();
            else delete this;
        }

        void read_handler(const boost::system::error_code& ec, size_t N) {
            //cout << "Session, read_handler, got: " << N << endl;
            if (ec.value() == 2) { start(); return; } // EOF
            if (ec) cout << "Session, read_handler failed with: " << ec.message() << endl;

            size_t gN = guard.size();
            if (!ec && N > gN) {
                string data;
                std::istream is(&buffer);
                std::istreambuf_iterator<char> it(is);
                copy_n( it, N-gN, std::back_inserter<std::string>(data) );
                for (int i=0; i<=gN; i++) it++;
                //data += "\n";
                //cout << "Session, received: " << data << ", cb: " << endl;
                if (onMessageCb) {
                    string answer = onMessageCb(data);
                    //cout << " send answer: " << answer << endl;
                    if (answer.size() > 0) {
                        auto cb = boost::bind(&Session::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
                        boost::asio::async_write(socket, boost::asio::buffer(answer, answer.size()), cb);
                    } else start();
                }
            } else {}
        }
};


class TCPServer {
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        vector<Session*> sessions;
        unique_ptr<tcp::acceptor> acceptor;
        thread waiting;
        thread service;
        string guard;

        function<string (string)> onMessageCb;

        template <typename Itr, typename Out>
        void copy_n(Itr it, size_t count, Out out) {
            for (size_t i=0; i<count; i++) out = *it++;
        }

        void waitFor() {
            Session* s = new Session(io_service, guard, onMessageCb);
            sessions.push_back(s);
            acceptor->async_accept(s->socket, [this,s](boost::system::error_code ec) { if (!ec) { s->start(); waitFor(); } });
        }

		void run() {
			io_service.run();
		}

    public:
        TCPServer() : worker(io_service) {
            service = thread([this](){ run(); });
        }

        ~TCPServer() { close(); }

        void onMessage( function<string (string)> f ) { onMessageCb = f; }

        void listen(int port, string guard) {
            cout << "TCPServer listen on port " << port << ", guard: " << guard << endl;
            this->guard = guard;
            if (!acceptor) acceptor = unique_ptr<tcp::acceptor>( new tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), port)) );
            waitFor();
        }

        void close() {
            io_service.stop();
            for (auto s : sessions) delete s;
            if (service.joinable()) service.join();
        }
};

VRTCPServer::VRTCPServer(string n) : name(n) { server = new TCPServer(); }
VRTCPServer::~VRTCPServer() { delete server; }

VRTCPServerPtr VRTCPServer::create(string name) { return VRTCPServerPtr(new VRTCPServer(name)); }

void VRTCPServer::onMessage( function<string(string)> f ) { server->onMessage(f); }
void VRTCPServer::listen(int port, string guard) { this->port = port; server->listen(port, guard); }
void VRTCPServer::close() { server->close(); }
int VRTCPServer::getPort() { return port; }

string VRTCPServer::getPublicIP() {
    if (publicIP != "") return publicIP;
    publicIP = VRTCPUtils::getPublicIP();
    return publicIP;
}

