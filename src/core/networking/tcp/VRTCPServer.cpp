#include "VRTCPServer.h"

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

class TCPServer {
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        tcp::socket socket;
        unique_ptr<tcp::acceptor> acceptor;
        thread waiting;
        thread service;
        boost::asio::streambuf buffer;
        string guard;

        function<string (string)> onMessageCb;

        template <typename Itr, typename Out>
        void copy_n(Itr it, size_t count, Out out) {
            for (size_t i=0; i<count; i++) out = *it++;
        }

        void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
            // done cb;
        }

        void read_handler(const boost::system::error_code& ec, size_t N) {
            //cout << "TCPServer, read_handler, got: " << N << endl;
            //if (ec == boost::system::error_code(2)) return; // EOF
            if (ec) cout << "TCPServer, read_handler failed with: " << ec << endl;

            size_t gN = guard.size();
            if (!ec && N > gN) {
                string data;
                std::istream is(&buffer);
                std::istreambuf_iterator<char> it(is);
                copy_n( it, N-gN, std::back_inserter<std::string>(data) );
                for (int i=0; i<gN; i++) it++;
                //data += "\n";
                //cout << "TCPServer, received: " << data << ", cb: " << endl;
                if (onMessageCb) {
                    string answer = onMessageCb(data);
                    //cout << " send answer: " << answer << endl;
                    if (answer.size() > 0) {
                        auto cb = boost::bind(&TCPServer::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
                        boost::asio::async_write(socket, boost::asio::buffer(answer, answer.size()), cb);
                    }
                }
                serve();
            } else {}
        }

        void serve() {
            if (guard == "") boost::asio::async_read( socket, buffer, boost::asio::transfer_at_least(1), bind(&TCPServer::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
            else boost::asio::async_read_until( socket, buffer, guard, bind(&TCPServer::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
        }

        void waitFor() {
            acceptor->async_accept(socket, [this](boost::system::error_code ec) { if (!ec) serve(); /*waitFor();*/ });
        }

		void run() {
			io_service.run();
		}

    public:
        TCPServer() : worker(io_service), socket(io_service) {
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
            socket.cancel();
            boost::system::error_code _error_code;
            socket.shutdown(tcp::socket::shutdown_both, _error_code);
            if (service.joinable()) service.join();
        }
};

VRTCPServer::VRTCPServer() { server = new TCPServer(); }
VRTCPServer::~VRTCPServer() { delete server; }

VRTCPServerPtr VRTCPServer::create() { return VRTCPServerPtr(new VRTCPServer()); }

void VRTCPServer::onMessage( function<string(string)> f ) { server->onMessage(f); }
void VRTCPServer::listen(int port, string guard) { this->port = port; server->listen(port, guard); }
void VRTCPServer::close() { server->close(); }
int VRTCPServer::getPort() { return port; }

//#ifndef _WINDOWS // under windows this only returns local network IP
string VRTCPServer::getPublicIP() {
    if (publicIP != "") return publicIP;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);

    const char* kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    connect(sock, (const sockaddr*) &serv, sizeof(serv));

    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(sock, (sockaddr*) &name, &namelen);

    char addressBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &name.sin_addr, addressBuffer, INET_ADDRSTRLEN);
#ifndef _WINDOWS
	::close(sock);
#else
    closesocket(sock);
#endif

    publicIP = string(addressBuffer);
    return publicIP;
}
/*#else
#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
string VRTCPServer::getPublicIP() {
    if (publicIP != "") return publicIP;

    HINTERNET net = InternetOpen("IP retriever", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET conn = InternetOpenUrl(net, "http://myexternalip.com/raw", NULL, 0, INTERNET_FLAG_RELOAD, 0);

    char buffer[4096];
    DWORD read;

    InternetReadFile(conn, buffer, sizeof(buffer) / sizeof(buffer[0]), &read);
    InternetCloseHandle(net);

    publicIP = std::string(buffer, read);
    return publicIP;
}
#endif*/

