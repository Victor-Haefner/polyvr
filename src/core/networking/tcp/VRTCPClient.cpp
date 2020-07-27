#include "VRTCPClient.h"

#include <boost/asio.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>
#include <memory>
#include <list>

using namespace OSG;
using namespace boost::asio;
using ip::tcp;

class TCPClient {
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        tcp::socket socket;
        list<string> messages;
        thread service;

        vector<boost::asio::ip::tcp::endpoint> uriToEndpoints(const string& uri) {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(uri, "");
            vector<boost::asio::ip::tcp::endpoint> res;
            for(boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query); i != boost::asio::ip::tcp::resolver::iterator(); ++i) {
                res.push_back(*i);
            }
            return res;
        }

        void processQueue() {
            boost::asio::async_write(socket, boost::asio::buffer(messages.front().data(), messages.front().length()),
                                    [this](boost::system::error_code ec, size_t N) {
                    if (!ec) {
                        messages.pop_front();
                        if (!messages.empty()) processQueue();
                    } else {
                        cout << " tcp client write ERROR: " << ec << "  N: " << N << endl;
                        socket.close();
                    }
                });
        }

    public:
        TCPClient() : worker(io_service), socket(io_service) {
            service = thread([this](){ io_service.run(); });
        }

        ~TCPClient() { close(); }

        void close() {
            io_service.stop();
            socket.cancel();
            boost::system::error_code _error_code;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _error_code);
            if (service.joinable()) service.join();
        }

        void connect(string host, int port) {
            socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(host), port ));
        }

        void connect(string uri) {
            socket.connect( uriToEndpoints(uri)[0] );
        }

        void send(string msg) {
            msg += "TCPPVR\n";
            bool write_in_progress = !messages.empty();
            messages.push_back(msg);
            if (!write_in_progress) processQueue();
        }
};


VRTCPClient::VRTCPClient() { client = new TCPClient(); }
VRTCPClient::~VRTCPClient() { delete client; }

VRTCPClientPtr VRTCPClient::create() { return VRTCPClientPtr(new VRTCPClient()); }

void VRTCPClient::connect(string host, int port) { client->connect(host, port); }
void VRTCPClient::connect(string host) { client->connect(host); }
void VRTCPClient::send(const string& message) { client->send(message); }





