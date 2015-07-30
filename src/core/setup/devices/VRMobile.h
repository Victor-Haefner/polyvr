#ifndef VRMOBILE_H_INCLUDED
#define VRMOBILE_H_INCLUDED

#include "VRDevice.h"

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSocket;

class VRMobile : public VRDevice {
    private:
        int port;
        VRSocket* soc;
        VRFunction<void*>* cb;

        map<string, string> websites;

        void callback(void* args);
        void updateMobilePage();

    public:
        VRMobile(int port);

        void clearSignals();

        void setPort(int port);
        int getPort();

        void remWebSite(string uri);
        void addWebSite(string uri, string website);
        void updateClients(string uri);

        void answerWebSocket(int id, string msg);
};


OSG_END_NAMESPACE;

#endif // VRMOBILE_H_INCLUDED
