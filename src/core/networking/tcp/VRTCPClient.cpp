#include "VRTCPClient.h"
#include "core/utils/toString.h"

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
        boost::asio::streambuf buffer;
        thread service;
        string guard;

        function<void (string)> onMessageCb;

        vector<boost::asio::ip::tcp::endpoint> uriToEndpoints(const string& uri) {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(uri, "");
            vector<boost::asio::ip::tcp::endpoint> res;
            for(boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query); i != boost::asio::ip::tcp::resolver::iterator(); ++i) {
                res.push_back(*i);
            }
            return res;
        }

        void read_handler(const boost::system::error_code& ec, size_t N) {
            //cout << "TCPClient, read_handler, got: " << N << endl;
            if (ec) cout << "TCPClient, read_handler failed with: " << ec << endl;

            size_t gN = guard.size();
            if (!ec && N > gN) {
                string data;
                std::istream is(&buffer);
                std::istreambuf_iterator<char> it(is);
                copy_n( it, N-gN, std::back_inserter<std::string>(data) );
                for (int i=0; i<=gN; i++) it++;
                //data += "\n";
                //cout << "TCPClient, received: " << data << ", cb: " << endl;
                if (onMessageCb) onMessageCb(data);
            } else {}
        }

        void processQueue() {
            boost::asio::async_write(socket, boost::asio::buffer(messages.front().data(), messages.front().length()),
                                    [this](boost::system::error_code ec, size_t N) {
                    if (!ec) {
                        messages.pop_front();
                        if (!messages.empty()) processQueue();
                        else {
                            if (guard == "") boost::asio::async_read( socket, buffer, boost::asio::transfer_at_least(1), bind(&TCPClient::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
                            else boost::asio::async_read_until( socket, buffer, guard, bind(&TCPClient::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
                        }
                    } else {
                        cout << " tcp client write ERROR: " << ec << "  N: " << N << endl;
                        socket.close();
                    }
                });
        }

		void run() {
			io_service.run();
		}

    public:
        TCPClient() : worker(io_service), socket(io_service) {
			service = thread([this]() { run(); });
        }

        ~TCPClient() { close(); }

        void onMessage( function<void (string)> f ) { onMessageCb = f; }

        void close() {
            io_service.stop();
            socket.cancel();
            boost::system::error_code _error_code;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _error_code);
            if (service.joinable()) service.join();
        }

        void connect(string host, int port) {
            cout << "TCPClient::connect to: " << host << ", on port " << port << endl;
            try {
                socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(host), port ));
            } catch(exception& e) {
                cout << "TCPClient::connect failed with: " << e.what() << endl;
            }
        }

        void connect(string uri) {
            cout << "TCPClient::connect to: " << uri << endl;
            try {
                socket.connect( uriToEndpoints(uri)[0] );
            } catch(exception& e) {
                cout << "TCPClient::connect failed with: " << e.what() << endl;
            }
        }

        void send(string msg, string guard) {
            this->guard = guard;
            msg += guard;
            bool write_in_progress = !messages.empty();
            messages.push_back(msg);
            if (!write_in_progress) processQueue();
        }

        bool connected() {
            return socket.is_open();
        }
};


VRTCPClient::VRTCPClient() { client = new TCPClient(); }
VRTCPClient::~VRTCPClient() { delete client; }

VRTCPClientPtr VRTCPClient::create() { return VRTCPClientPtr(new VRTCPClient()); }

void VRTCPClient::connect(string host, int port) { client->connect(host, port); }
void VRTCPClient::connect(string host) { client->connect(host); }
void VRTCPClient::send(const string& message, string guard) { client->send(message, guard); }
bool VRTCPClient::connected() { return client->connected(); }

void VRTCPClient::onMessage( function<void(string)> f ) { client->onMessage(f); }




