#ifndef VRMOBILE_H_INCLUDED
#define VRMOBILE_H_INCLUDED

#include "VRDevice.h"

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSocket;

class VRServer : public VRDevice {
    private:
        int port = 5500;
        VRSocket* soc = 0;
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

        void answerWebSocket(int id, string msg);
};


OSG_END_NAMESPACE;

#endif // VRMOBILE_H_INCLUDED
