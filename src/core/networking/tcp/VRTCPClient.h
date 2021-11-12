#ifndef VRTCPCLIENT_H_INCLUDED
#define VRTCPCLIENT_H_INCLUDED

#include "../VRNetworkingFwd.h"

#include <string>
#include <functional>

class TCPClient;

using namespace std;
namespace OSG {

class VRTCPClient {
    private:
        string uri;
        TCPClient* client = 0;

    public:
        VRTCPClient();
        ~VRTCPClient();

        static VRTCPClientPtr create();

        void onConnect( function<void(void)>   f );
        void onMessage( function<void(string)> f );
        void setGuard( string g );

        void connect(string host, int port);
        void connect(string uri);
        void connectToPeer(int localPort, string remoteIP, int remotePort);
        void send(const string& message, string guard = "");
        bool connected();

        string getPublicIP(bool cached = false);
        string getConnectedUri();
};

}

#endif // VRTCPCLIENT_H_INCLUDED
