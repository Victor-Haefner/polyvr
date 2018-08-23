#ifndef VRMOBILE_H_INCLUDED
#define VRMOBILE_H_INCLUDED

#include "VRDevice.h"
#include "core/networking/VRNetworkingFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSocket;

class VRServer : public VRDevice {
    private:
        int port = 5500;
        VRSocketPtr soc = 0;
        VRFunction<void*>* cb = 0;

        map<string, string> websites;
        map<string, VRServerCbPtr> callbacks;

        void callback(void* args);
        void updateMobilePage();

    public:
        VRServer(int port = 5500);
        ~VRServer();

        static VRServerPtr create(int port = 5500);
        VRServerPtr ptr();

        void clearSignals();

        void setPort(int port);
        int getPort();

        void addCallback(string path, VRServerCbPtr cb);
        void remCallback(string path);

        void remWebSite(string uri);
        void addWebSite(string uri, string website);
        void updateClients(string uri);

        int openWebSocket(string address, string protocols);
        void answerWebSocket(int id, string msg);

        map<string, vector<int>> getClients();
};


OSG_END_NAMESPACE;

#endif // VRMOBILE_H_INCLUDED
