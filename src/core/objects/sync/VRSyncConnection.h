#ifndef VRSYNCCONNECTION_H_INCLUDED
#define VRSYNCCONNECTION_H_INCLUDED

#include "core/networking/VRNetworkingFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"

#include <OpenSG/OSGBaseTypes.h>
#include <OpenSG/OSGFieldContainerFactory.h>

#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRSyncConnection);

class VRSyncConnection {
    public:
        struct Avatar {
            VRTransformPtr head;
            VRTransformPtr dev;
            VRTransformPtr anchor;
            UInt32 localHeadID = 0;
            UInt32 localDevID = 0;
            UInt32 localAnchorID = 0;
        };

    private:
        // TODO: unused, currently handled in syncnode, needs to move here
        map<UInt32, UInt32> typeMapping;
        map<UInt32, UInt32> remoteToLocalID;
        map<UInt32, UInt32> localToRemoteID;
        map<UInt32, UInt32> remoteCoreToLocalNode;
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        Avatar avatar;

        string uri;
        string localUri;
        VRTCPClientPtr client;
        VRTimerPtr timer;

        FieldContainerFactoryBase* factory = FieldContainerFactory::the();

    public:
        VRSyncConnection(string host, int port, string localUri);
        VRSyncConnection(VRTCPClientPtr client, string localUri);
        ~VRSyncConnection();

        static VRSyncConnectionPtr create(string host, int port, string localUri);
        static VRSyncConnectionPtr create(VRTCPClientPtr client, string localUri);

        void connect();
        bool send(string message);
        void startInterface(int port, VRSyncNodePtr snode);
        void keepAlive();

        string getStatus();
        string getUri();
        string getLocalUri();

        Avatar& getAvatar();
        void setupDevices(UInt32 headTransform, UInt32 devTransform, UInt32 devAnchor);
        void setupAvatar(VRTransformPtr headTransform, VRTransformPtr devTransform, VRTransformPtr devAnchor);
        void handleAvatar(string data);
        void updateAvatar(string data);
        UInt32 getNodeID(VRObjectPtr t);
        UInt32 getTransformID(VRTransformPtr t);

        static string base64_encode(unsigned char const* buf, UInt32 bufLen);
        static vector<unsigned char> base64_decode(string const& encoded_string);

        void addRemoteMapping(UInt32 lID, UInt32 rID);
        void handleTypeMapping(string mappingData);
        void handleMapping(string mappingData);
        void handleRemoteMapping(string mappingData, VRSyncNodePtr syncNode);
        UInt32 getRemoteID(UInt32 id);
        UInt32 getLocalID(UInt32 id);
        UInt32 getLocalType(UInt32 id);

        bool isRemoteChange(const UInt32& id);
        void logSyncedContainer(UInt32 id);
        void clearSyncedContainer();
};

OSG_END_NAMESPACE;

#endif // VRSYNCCONNECTION_H_INCLUDED
