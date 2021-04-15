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
        typedef shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

        boost::asio::io_service io_service;
        boost::asio::io_service::work worker;
        SocketPtr socket;
        list<string> messages;
        boost::asio::streambuf buffer;
        thread service;
        string guard;

        // hole punching
        thread tunnelAccept;
        thread tunnelConnect;
        thread tunnelRead;
        SocketPtr aSocket;
        SocketPtr cSocket;

        function<void (string)> onMessageCb;
        function<void (void)> onConnectCb;

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
            if (ec) cout << "TCPClient, read_handler failed with: " << ec.message() << endl;

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

        void read(bool block = false) {
            if (!block) {
                if (guard == "") boost::asio::async_read( *socket, buffer, boost::asio::transfer_at_least(1), bind(&TCPClient::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
                else boost::asio::async_read_until( *socket, buffer, guard, bind(&TCPClient::read_handler, this, std::placeholders::_1, std::placeholders::_2) );
            } else {
                boost::system::error_code ec;
                size_t N = 0;
                if (guard == "") N = boost::asio::read( *socket, buffer, boost::asio::transfer_at_least(1), ec);
                else N = boost::asio::read_until( *socket, buffer, guard, ec);
                read_handler(ec, N);
            }
        }

        void processQueue() {
            boost::asio::async_write(*socket, boost::asio::buffer(messages.front().data(), messages.front().length()),
                                    [this](boost::system::error_code ec, size_t N) {
                    if (!ec) {
                        messages.pop_front();
                        if (!messages.empty()) processQueue();
                        else read();
                    } else {
                        cout << " tcp client write ERROR: " << ec << "  N: " << N << endl;
                        socket->close();
                    }
                });
        }

    public:
        TCPClient() : worker(io_service) {
            socket = SocketPtr( new tcp::socket(io_service) );
			service = thread([this]() { io_service.run(); });
        }

        ~TCPClient() { close(); }

        void onMessage( function<void (string)> f ) { onMessageCb = f; }
        void onConnect( function<void (void)> f ) { onConnectCb = f; }

        void close() {
            io_service.stop();
            socket->cancel();
            boost::system::error_code _error_code;
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, _error_code);
            if (service.joinable()) service.join();
        }

        void connect(string host, int port) {
            //cout << "TCPClient::connect to: " << host << ", on port " << port << endl;
            try {
                socket->connect( tcp::endpoint( boost::asio::ip::address::from_string(host), port ));
            } catch(exception& e) {
                cout << "TCPClient::connect failed with: " << e.what() << endl;
            }
        }

        void connect(string uri) {
            //cout << "TCPClient::connect to: " << uri << endl;
            try {
                socket->connect( uriToEndpoints(uri)[0] );
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
            return socket->is_open();
        }

        void connectToPeer(string lIP, int lPort, string rIP, int rPort) {
            //cout << "TCPClient::connectToPeer " << lIP << ":" << lPort << ", to " << rIP << ":" << rPort << endl;
            tunnelAccept = thread([this, lPort]() { acceptHolePunching(lPort); });
            tunnelConnect = thread([this, lIP, lPort, rIP, rPort]() { connectHolePunching(lIP, lPort, rIP, rPort); });
		}

		void finalizeP2P() {
            //tunnelAccept.join(); // TODO
            //tunnelConnect.join();

            //cout << " p2p socket states: " << aSocket->is_open() << " " << cSocket->is_open() << endl;

            if (aSocket->is_open()) socket = aSocket;
            if (cSocket->is_open()) socket = cSocket;

            tunnelRead = thread([this]() { while(true) read(true); });

            if (onConnectCb) onConnectCb();
		}

        void acceptHolePunching(int port) {
            //cout << "TCPClient::acceptHolePunching on " << port << endl;
            bool stop = false;
            boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);
            boost::asio::ip::tcp::acceptor acceptor(io_service, ep.protocol());

            //cout << " accept acceptor bind on endpoint " << ep << endl;
            boost::asio::socket_base::reuse_address reuseAddress(true);
            boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reusePort(true);
            acceptor.set_option(reuseAddress); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            acceptor.set_option(reusePort); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

            boost::system::error_code ec;
            acceptor.bind(ep, ec); //    s.bind(('', port))

            //Handling Errors
            if (ec != 0) {
                std::cout << "Failed to bind the acceptor socket." << "Error code = " << ec.value() << ". Message: " << ec.message() << endl;
            }

            acceptor.listen(1); //    s.listen(1)
        //    s.settimeout(5)
            aSocket = SocketPtr( new tcp::socket(io_service) );//    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            while (!stop) {  //    while not STOP.is_set():
                bool exception_caught = true;
                try {  //        try:
                    //cout << " try accept on " << port << endl;
                    acceptor.accept(*aSocket); //            conn, addr = s.accept()
                    //cout << "  --- accepted connection! ---" << endl;
                }
                catch (boost::system::system_error& e) { //        except socket.timeout:
                    //cout << "Exception at VRSyncConnection::connect2. Exception Nr. " << e.message() << endl;
                    continue; //            continue
                }
                if (!exception_caught) { //        else:
                    stop = true; //            STOP.set()
                    finalizeP2P();
                }
            }
        }

        void connectHolePunching(string localIP, int localPort, string remoteIP, int remotePort) {
            sleep(1);
            //cout << "TCPClient::connectHolePunching from " << localIP << ":" << localPort << ", to " << remoteIP << ":" << remotePort << endl;
            bool stop = false;
            boost::asio::ip::tcp::endpoint local_ep(boost::asio::ip::address::from_string(localIP), localPort);

            cSocket = SocketPtr( new tcp::socket(io_service, local_ep.protocol()) );//    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            //cout << " connect socket bind " << endl;
            boost::asio::socket_base::reuse_address reuseAddress(true);
            boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reusePort(true);
            cSocket->set_option(reuseAddress); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            cSocket->set_option(reusePort); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

            boost::system::error_code ec;
            cSocket->bind(local_ep, ec); //    s.bind(local_addr)

            if (ec != 0) { //Handling Errors
                std::cout << "Failed to bind the socket." << "Error code = " << ec.value() << ". Message: " << ec.message() << endl;
            }

            auto remoteAddr = boost::asio::ip::address::from_string(remoteIP);
            boost::asio::ip::tcp::endpoint remote_ep(remoteAddr, remotePort);

            while (!stop) { //while not STOP.is_set():
                bool exception_caught = true;
                try {//        try:
                    //cout << "VRTCPClient::connectHolePunching trying " << this << " " << remoteIP << ":" << remotePort << endl;
                    cSocket->connect(remote_ep);//            s.connect(addr)
                    //cout << " --- VRTCPClient::connectHolePunching connect ---" << endl;
                    exception_caught = false;
                } catch (boost::system::system_error& e) {
                    //cout << "boost::system::system_error " << e.what() << endl; // mostly "No route to host", which is Ok
                    continue;
                } catch (...) {
                    cout << "Unknown Exception raised!" << endl;
                    continue;
                }
                if (!exception_caught) {//        else:
                    stop = true;//            STOP.set()
                    finalizeP2P();
                }
            }
        }
};


VRTCPClient::VRTCPClient() { client = new TCPClient(); }
VRTCPClient::~VRTCPClient() { delete client; }

VRTCPClientPtr VRTCPClient::create() { return VRTCPClientPtr(new VRTCPClient()); }

void VRTCPClient::connect(string host, int port) { client->connect(host, port); }
void VRTCPClient::connect(string host) { client->connect(host); }
void VRTCPClient::send(const string& message, string guard) { client->send(message, guard); }
bool VRTCPClient::connected() { return client->connected(); }

void VRTCPClient::onConnect( function<void(void)>   f ) { client->onConnect(f); }
void VRTCPClient::onMessage( function<void(string)> f ) { client->onMessage(f); }

void VRTCPClient::connectToPeer(string localIP, int localPort, string remoteIP, int remotePort) {
    client->connectToPeer(localIP, localPort, remoteIP, remotePort);
}



