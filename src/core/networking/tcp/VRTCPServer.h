#ifndef VRTCPSERVER_H_INCLUDED
#define VRTCPSERVER_H_INCLUDED

#include "../VRNetworkServer.h"

#include <string>
#include <functional>

class TCPServer;

using namespace std;
namespace OSG {

class VRTCPServer : public VRNetworkServer {
    private:
        TCPServer* server = 0;
        string publicIP = "";

    public:
        VRTCPServer(string name);
        ~VRTCPServer();

        static VRTCPServerPtr create(string name = "none");

        void onMessage( function<string(string)> f );

        void listen(int port, string guard = "");
        void close();

        string getPublicIP();
        int getPort();
};

}

#endif // VRTCPSERVER_H_INCLUDED
