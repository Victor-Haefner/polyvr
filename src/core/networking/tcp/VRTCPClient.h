#ifndef VRTCPCLIENT_H_INCLUDED
#define VRTCPCLIENT_H_INCLUDED

#include "../VRNetworkClient.h"

class TCPClient;

using namespace std;
namespace OSG {

class VRTCPClient : public VRNetworkClient {
    private:
        TCPClient* client = 0;

    public:
        VRTCPClient(string name);
        ~VRTCPClient();

        static VRTCPClientPtr create(string name = "none");

        void onConnect( function<void(void)>   f );
        void onMessage( function<string(string)> f ) override;
        void setGuard( string g );

        void connect(string host, int port) override;
        bool isConnected(string host, int port) override;
        void connect(string uri);
        void connectToPeer(int localPort, string remoteIP, int remotePort);
        void send(const string& message, string guard = "", bool verbose = false) override;
        bool connected();
        void close();

        string getPublicIP(bool cached = false);
};

}

#endif // VRTCPCLIENT_H_INCLUDED
