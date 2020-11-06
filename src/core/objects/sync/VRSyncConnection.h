#ifndef VRSYNCCONNECTION_H_INCLUDED
#define VRSYNCCONNECTION_H_INCLUDED

#include "core/networking/VRNetworkingFwd.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGBaseTypes.h>

#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRSyncConnection);

class VRSyncConnection {
    private:
        // TODO: unused, currently handled in syncnode, needs to move here
        map<UInt32, UInt32> fcMapping; // <remote container ID, local container ID>
        map<UInt32, UInt32> typeMapping; // <remote type ID, local type ID>
        string host;
        int port;
        VRTCPClientPtr client;

    public:
        VRSyncConnection(string host, int port);
        ~VRSyncConnection();
        static VRSyncConnectionPtr create(string host, int port);

        void connect();
        bool send(string message);
        void startInterface(int port, VRSyncNodePtr snode);

        string getStatus();

        static string base64_encode(unsigned char const* buf, UInt32 bufLen);
        static vector<unsigned char> base64_decode(string const& encoded_string);
};

OSG_END_NAMESPACE;

#endif // VRSYNCCONNECTION_H_INCLUDED
