#ifndef VRTCPSERVER_H_INCLUDED
#define VRTCPSERVER_H_INCLUDED

#include "../VRNetworkingFwd.h"

#include <functional>

class TCPServer;

using namespace std;
namespace OSG {

class VRTCPServer {
    private:
        TCPServer* server = 0;
        int port = 0;

    public:
        VRTCPServer();
        ~VRTCPServer();

        static VRTCPServerPtr create();

        void onMessage( function<void (string)> f );

        void listen(int port);
        void close();

        string getPublicIP();
        int getPort();
};

}

#endif // VRTCPSERVER_H_INCLUDED
