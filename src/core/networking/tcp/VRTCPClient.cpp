#include "VRTCPClient.h"
#include "VRTCPUtils.h"
#include "core/utils/toString.h"
#include "core/utils/VRMutex.h"
#include "core/gui/VRGuiConsole.h"
#include "core/scene/VRSceneManager.h"

#include "asio.hpp"

#include <cstdlib>
#include <iostream>
#include "core/utils/Thread.h"
#include <string>
#include <memory>
#include <list>

using namespace OSG;
using asio::ip::tcp;

// SO_REUSEPORT is undeclared in windows ??
#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

class TCPClient {
    private:
        typedef std::shared_ptr<tcp::socket> SocketPtr;
        typedef std::shared_ptr<tcp::acceptor> AcceptorPtr;

        VRTCPClient* parent = 0;

        asio::io_context io_service;
        asio::executor_work_guard<asio::io_context::executor_type> worker;

        SocketPtr socket;
        list<string> messages;
        asio::streambuf buffer;
        ::Thread* service = 0;
        string guard;
        VRMutex mtx;

        // hole punching
        ::Thread* tunnelAccept = 0;
        ::Thread* tunnelConnect = 0;
        ::Thread* tunnelRead = 0;
        SocketPtr aSocket;
        SocketPtr cSocket;
        AcceptorPtr acceptor;
        bool stop = false;
        bool broken = false;

        function<string(string)> onMessageCb;
        function<void (void)> onConnectCb;

        vector<tcp::endpoint> uriToEndpoints(string uri) {
            string port = "";
            if (contains(uri, ":")) {
                port = splitString(uri, ':')[1];
                uri  = splitString(uri, ':')[0];
            }

            vector<tcp::endpoint> res;
            try {
                tcp::resolver resolver(io_service);
                tcp::resolver::results_type endpoints = resolver.resolve(uri, port);
                for (const auto& e : endpoints) {
                    res.push_back(e.endpoint());
                }
            } catch(std::system_error& e) {
                cout << "Exception at TCPClient::uriToEndpoints: " << e.what() << endl;
            }
            return res;
        }

        bool read_handler(const std::error_code& ec, size_t N) {
            if (parent) {
                auto& iFlow = parent->getInFlow();
                iFlow.logFlow(N*0.001);
            }

            //cout << "TCPClient, " << this << " read_handler, got: " << N << endl;
            if (ec) {
                cout << "TCPClient, read_handler failed with: " << ec.message() << endl;
                broken = true;
                return false;
            }

            size_t gN = guard.size();
            //cout << " TCPClient, guard: " << gN << " " << guard << endl;
            if (!ec && N > gN) {
                string data;
                std::istream is(&buffer);
                std::istreambuf_iterator<char> it(is);
                copy_n( it, N-gN, std::back_inserter<std::string>(data) );
                for (size_t i=0; i<=gN; i++) it++;
                //data += "\n";
                //cout << "  TCPClient, received: " << data.size() << " " << data << " " << int(data[0]) << endl;
                if (onMessageCb) onMessageCb(data);
            } else {}

            return true;
        }

        bool read(bool block = false) {
            if (broken) return false;

            //cout << "TCPClient, " << this << " read " << (block?"blocking":"non blocking") << endl;
            if (!block) {
                auto onRead = [this](std::error_code ec, size_t N) {
                    read_handler(ec, N);
                    read();
                };

                if (guard == "") asio::async_read( *socket, buffer, asio::transfer_at_least(1), onRead );
                else asio::async_read_until( *socket, buffer, guard, onRead );
                return true;
            } else {
                std::error_code ec;
                size_t N = 0;
                if (guard == "") N = asio::read( *socket, buffer, asio::transfer_at_least(1), ec);
                else N = asio::read_until( *socket, buffer, guard, ec);
                return read_handler(ec, N);
            }
        }

        void processQueue() {
            auto onWritten = [this](std::error_code ec, size_t N) { // write queued messages until done, then go read
                if (parent) {
                    auto& oFlow = parent->getOutFlow();
                    oFlow.logFlow(N*0.001);
                }
                //cout << " async write finished " << N << endl;
                if (!ec) {
                    VRLock lock(mtx);
                    this->messages.pop_front();
                    if (!this->messages.empty()) processQueue();
                } else {
                    cout << " tcp client write ERROR: " << ec.message() << "  N: " << N << ", close socket!" << endl;
                    socket->close();
                }
            };

            if (broken) return;
            VRLock lock(mtx);
            asio::async_write(*socket, asio::buffer(this->messages.front().data(), this->messages.front().length()), onWritten );
        }

        void runService() { // needed because windows complains..
            try {
                io_service.run();
            } catch(...) {
                cout << "Error in TCPClient::runService!" << endl;
                runService();
            }
        }

    public:
        TCPClient(VRTCPClient* p) : parent(p), worker(asio::make_work_guard(io_service)) {
            socket = SocketPtr( new tcp::socket(io_service) );
            service = new ::Thread("TCPClient_service", [this]() { runService(); });
        }

        ~TCPClient() { close(); }

        void onMessage( function<string(string)> f ) { onMessageCb = f; }
        void onConnect( function<void (void)> f ) { onConnectCb = f; }

        void close() {
            cout << "close TCP client" << endl;
            stop = true;

            try {
                io_service.stop();
                if (aSocket) aSocket->close();
                if (cSocket) cSocket->close();
                socket->close();
                std::error_code _error_code;
                socket->shutdown(tcp::socket::shutdown_both, _error_code);
            } catch(...) {
                cout << "Error in TCPClient::close!" << endl;
            }

            cout << "join service thread" << endl;
            if (service->joinable()) service->join();
            delete service;

            if (tunnelAccept) {
                cout << "join hp accept thread" << endl;
                if (tunnelAccept->joinable()) tunnelAccept->join();
                delete tunnelAccept;
            }

            if (tunnelConnect) {
                cout << "join hp connect thread" << endl;
                if (tunnelConnect->joinable()) tunnelConnect->join();
                delete tunnelConnect;
            }

            if (tunnelRead) {
                cout << "join hp read thread" << endl;
                if (tunnelRead->joinable()) tunnelRead->join();
                delete tunnelRead;
            }
        }

        void connect(string host, int port) {
            cout << "TCPClient::connect " << this << " to: " << host << ", on port " << port << endl;
            try {
                socket->connect(tcp::endpoint(asio::ip::make_address(host), port));
                read();
            } catch(std::exception& e) {
                cout << "TCPClient::connect failed with: " << e.what() << endl;
#ifndef WITHOUT_IMGUI
                VRConsoleWidget::get("Collaboration")->write( " TCP connect to "+host+":"+toString(port)+" failed with "+e.what()+"\n", "red");
#endif
            }
        }

        void connect(string uri) {
            //cout << "TCPClient::connect to: " << uri << endl;
            auto endpoints = uriToEndpoints(uri);
            if (endpoints.size() == 0) {
                cout << "TCPClient::connect failed, no endpoints found for uri " << uri << endl;
#ifndef WITHOUT_IMGUI
                VRConsoleWidget::get("Collaboration")->write( " TCP connect to "+uri+" failed, no endpoints found\n", "red");
#endif
                return;
            }

            try {
                if (socket) socket->connect( endpoints[0] );
                read();
            } catch(std::exception& e) {
                cout << "TCPClient::connect failed with: " << e.what() << endl;
#ifndef WITHOUT_IMGUI
                VRConsoleWidget::get("Collaboration")->write( " TCP connect to "+uri+" ("+toString(endpoints.size())+" endpoints) failed with "+e.what()+"\n", "red");
#endif
            }
        }

        void setGuard(string g) { guard = g; }

        void send(string msg, string guard, bool verbose) {
            if (broken) return;
            VRLock lock(mtx);
            this->guard = guard;
            msg += guard;
            size_t S = msg.size();
            double s = S/1000.0;
            //if (verbose) cout << "TCPClient::send " << this << " msg: " << msg << ", " << s << " kb" << endl;
            if (verbose) cout << "TCPClient::send " << this << ", " << s << " kb" << endl;
            bool write_in_progress = !messages.empty();
            messages.push_back(msg);
            if (!write_in_progress) processQueue();
        }

        bool connected() {
            if (broken) return false;
            return socket->is_open();
        }

        void connectToPeer(int lPort, string rIP, int rPort) {
            string lIP = VRTCPUtils::getLocalIP();
            cout << "TCPClient::connectToPeer " << lIP << ":" << lPort << ", to " << rIP << ":" << rPort << endl;
            tunnelAccept = new ::Thread("tunnelAccept", [this, lPort]() { acceptHolePunching(lPort); }); // needed??? if yes, then TODO: fix close (timeout)!
            tunnelConnect = new ::Thread("tunnelConnect", [this, lIP, lPort, rIP, rPort]() { connectHolePunching(lIP, lPort, rIP, rPort); });
            //tunnelAccept.detach(); // TODO: implement timeout or other abort method to control that thread!
		}

		void finalizeP2P() {
            //cout << " p2p socket states: " << aSocket->is_open() << " " << cSocket->is_open() << endl;

            //if (aSocket->is_open()) socket = aSocket;
            if (cSocket->is_open()) socket = cSocket;
            if (acceptor) acceptor->cancel();
            aSocket.reset();
            cSocket.reset();
            acceptor.reset();

            cout << "finalizeP2P, close threads " << this << endl;
            //tunnelAccept.join();
            //tunnelConnect.join();
            cout << " finalizeP2P, close threads done" << endl;

            tunnelRead = new ::Thread("tunnelRead", [this]() { bool run = true; while(run) run = read(true); });

            if (onConnectCb) onConnectCb();
		}

        void acceptHolePunching(int port) {
            //cout << "TCPClient::acceptHolePunching on " << port << endl;
            tcp::endpoint ep(tcp::v4(), port);
            acceptor = AcceptorPtr( new tcp::acceptor(io_service, ep.protocol()) );

            //cout << " accept acceptor bind on endpoint " << ep << endl;
            asio::socket_base::reuse_address reuseAddress(true);
            asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reusePort(true);
            //asio::socket_base::enable_connection_aborted enable_abort(true);
            acceptor->set_option(reuseAddress); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            acceptor->set_option(reusePort); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            //acceptor->set_option(enable_abort);

            std::error_code ec;
            acceptor->bind(ep, ec); //    s.bind(('', port))

            //Handling Errors
            if (ec.value() != 0) {
                std::cout << "Failed to bind the acceptor socket." << "Error code = " << ec.value() << ". Message: " << ec.message() << endl;
            }

            acceptor->listen(1); //    s.listen(1)
            aSocket = SocketPtr( new tcp::socket(io_service) );//    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            //    s.settimeout(5)

            while (!stop) {  //    while not STOP.is_set():
                try {  //        try:
                    cout << " try accept on " << port << endl;
                    acceptor->accept(*aSocket); //            conn, addr = s.accept()
                    cout << "  --- accepted connection! ---" << endl;
                }
                catch (std::system_error& e) { //        except socket.timeout:
                    cout << "Exception at VRSyncConnection::connect2. Exception Nr. " << e.what() << endl;
                    continue; //            continue
                }
                //        else:
                stop = true; //            STOP.set()
                finalizeP2P();
            }
        }

        void connectHolePunching(string localIP, int localPort, string remoteIP, int remotePort) {
            //sleep(1);
            cout << "TCPClient::connectHolePunching from " << localIP << ":" << localPort << ", to " << remoteIP << ":" << remotePort << endl;
            tcp::endpoint local_ep(asio::ip::make_address(localIP), localPort);

            cSocket = SocketPtr( new tcp::socket(io_service, local_ep.protocol()) );//    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            cout << " connect socket bind " << endl;
            asio::socket_base::reuse_address reuseAddress(true);
            asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reusePort(true);
            cSocket->set_option(reuseAddress); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#ifndef _WIN32
            cSocket->set_option(reusePort); //    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
#endif

            std::error_code ec;
            cSocket->bind(local_ep, ec); //    s.bind(local_addr)

            if (ec.value() != 0) { //Handling Errors
                cout << "Error in VRTCPClient::connectHolePunching, failed to bind the socket to " << local_ep << "! Error code = " << ec.value() << " (" << ec.message() << ")" << endl;
            }

            asio::ip::address remoteAddr;
            try {
                remoteAddr = asio::ip::make_address(remoteIP);
            } catch(...) {
                cout << "Error in VRTCPClient::connectHolePunching, failed to parse remote IP '" << remoteIP << "'" << endl;
                return;
            }
            tcp::endpoint remote_ep(remoteAddr, remotePort);

            while (!stop) { //while not STOP.is_set():
                try { //        try:
                    cout << "VRTCPClient::connectHolePunching trying " << this << " " << remoteIP << ":" << remotePort << endl;
                    cSocket->connect(remote_ep);//            s.connect(addr)
                    cout << " --- VRTCPClient::connectHolePunching connect ---" << endl;
                } catch (std::system_error& e) {
                    //static int c = 0; c++;
                    //if (c > 20) return;

                    if (e.code().value() == 113) continue; // No route to host - is normal
                    if (e.code().value() == 111) continue; // Connection refused - very bad!
                    cout << "Error in VRTCPClient::connectHolePunching socket->connect, std::system_error " << e.what() << "(" << e.code().value() << ")" << endl;
                    return;
                } catch (...) {
                    cout << "Unknown Exception raised!" << endl;
                    return;
                }
                stop = true;//            STOP.set()
                finalizeP2P();
            }
        }
};


VRTCPClient::VRTCPClient(string name) : VRNetworkClient(name) { protocol = "tcp"; client = new TCPClient(this); }

VRTCPClient::~VRTCPClient() {
    delete client;
}

VRTCPClientPtr VRTCPClient::create(string name) {
    auto c = VRTCPClientPtr(new VRTCPClient(name));
    VRSceneManager::get()->regNetworkClient(c);
    return c;
}

void VRTCPClient::connect(string host, int port) { client->connect(host, port); uri = host+":"+toString(port); }
bool VRTCPClient::isConnected(string host, int port) { return bool(host+":"+toString(port) == uri); }
void VRTCPClient::connect(string host) { client->connect(host); uri = host; }
void VRTCPClient::setGuard(string guard) { client->setGuard(guard); }
void VRTCPClient::send(const string& message, string guard, bool verbose) { client->send(message, guard, verbose); }
bool VRTCPClient::connected() { return client->connected(); }

void VRTCPClient::onConnect( function<void(void)>   f ) { client->onConnect(f); }
void VRTCPClient::onMessage( function<string(string)> f ) { client->onMessage(f); }

void VRTCPClient::connectToPeer(int localPort, string remoteIP, int remotePort) {
    client->connectToPeer(localPort, remoteIP, remotePort);
}

void VRTCPClient::close() { // TODO: the onConnect and onMessage callbacks get lost here!
    delete client;
    client = new TCPClient(this);
}

string VRTCPClient::getPublicIP(bool cached) { return VRTCPUtils::getPublicIP(cached); }
