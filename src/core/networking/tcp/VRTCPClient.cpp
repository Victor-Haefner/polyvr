#include "VRTCPClient.h"

#include <boost/asio.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>
#include <memory>

using namespace OSG;
using namespace boost::asio;
using ip::tcp;

class TCPClient {
    private:
        boost::asio::io_service io_service;
        tcp::socket socket;

        vector<boost::asio::ip::tcp::endpoint> uriToEndpoints(const string& uri) {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(uri, "");
            vector<boost::asio::ip::tcp::endpoint> res;
            for(boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query); i != boost::asio::ip::tcp::resolver::iterator(); ++i) {
                res.push_back(*i);
            }
            return res;
        }

    public:
        TCPClient() : socket(io_service) {}
        ~TCPClient() {}

        void connect(string host, int port) {
            cout << "connect to " << host << " on " << port << endl;
            socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(host), port ));
        }

        void connect(string uri) {
            cout << "connect to " << uri << endl;
            socket.connect( uriToEndpoints(uri)[0] );
        }

        void send(string msg) {
            cout << " send data N:" << msg.size() << endl;
            msg += "TCPPVR\n";
            /*boost::asio::async_write(socket, boost::asio::buffer(msg.data(), msg.length()),
                                    [this, &msg](boost::system::error_code ec, size_t N) {
                                        cout << "  ERROR: " << ec << "  N: " << N << endl;
                                    });*/
            boost::asio::write(socket, boost::asio::buffer(msg.data(), msg.length()));
        }
};


VRTCPClient::VRTCPClient() { client = new TCPClient(); }
VRTCPClient::~VRTCPClient() { delete client; }

VRTCPClientPtr VRTCPClient::create() { return VRTCPClientPtr(new VRTCPClient()); }

void VRTCPClient::connect(string host, int port) { client->connect(host, port); }
void VRTCPClient::connect(string host) { client->connect(host); }
void VRTCPClient::send(const string& message) { client->send(message); }





