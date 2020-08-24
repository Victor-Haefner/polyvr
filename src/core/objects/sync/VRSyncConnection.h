#ifndef VRSYNCCONNECTION_H_INCLUDED
#define VRSYNCCONNECTION_H_INCLUDED

#include "core/networking/VRWebSocket.h"
#include <OpenSG/OSGBaseTypes.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRSyncConnection);

class VRSyncConnection {
    private:
        map<UInt32, UInt32> mapping; // <remote container ID, local container ID>
        string uri;
        VRWebSocketPtr socket;

    public:
        VRSyncConnection(string uri = "");
        ~VRSyncConnection();
        static VRSyncConnectionPtr create(string name = "None");

        void connect();
        bool send(string message);

        static string base64_encode(unsigned char const* buf, UInt32 bufLen);
        static vector<unsigned char> base64_decode(string const& encoded_string);
};

OSG_END_NAMESPACE;

#endif // VRSYNCCONNECTION_H_INCLUDED
