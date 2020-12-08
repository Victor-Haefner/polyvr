#ifndef VRSyncNode_H_INCLUDED
#define VRSyncNode_H_INCLUDED

#include "VRSyncConnection.h"
#include "VRSyncChangelist.h"
#include "core/objects/VRTransform.h"
#include "core/networking/VRNetworkingFwd.h"
#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGFieldContainerFactory.h>
//#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGContainerIdMapper.h>
#include "core/math/pose.h"

class OSGChangeList;
struct SerialEntry;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSyncNode : public VRTransform {
    private:
        typedef unsigned char BYTE;
        VRTCPServerPtr server;
        VRUpdateCbPtr updateFkt;
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        vector<UInt32> createdNodes; //IDs of the currently created nodes/children

        size_t selfID = 0;
        bool doWrapping = true;
        bool doAvatars = true;
        bool handledPoses = false; // optimization

        VRMessageCbPtr onEvent;

        VRSyncChangelistPtr changelist;

        map<UInt32, UInt32> container; // local containers, sub-set of containers which need to be synced for collaboration
        map<UInt32, UInt32> externalContainer; // local external containers, key is container ID, value is change mask to use
        //vector<UInt32> cores; //lists IDs of nodecores
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        map<string, VRSyncConnectionPtr> remotes;
        map<UInt32, UInt32> remoteToLocalID;
        map<UInt32, UInt32> localToRemoteID;
        map<UInt32, UInt32> typeMapping;
        map<UInt32, UInt32> remoteCoreToLocalNode;
        map<UInt32, VRObjectWeakPtr> nodeToVRObject;
        UInt32 getRegisteredContainerID(UInt32 syncID);
        UInt32 getRegisteredSyncID(UInt32 fieldContainerID);
        bool isRegisteredRemote(const UInt32& syncID);
        void getAllSubContainersRec(FieldContainer* node, FieldContainer* parent, map<FieldContainer*, vector<FieldContainer*>>& res);
        UInt32 findParent(map<UInt32,vector<UInt32>>& parentToChildren, UInt32 remoteNodeID);

        VRObjectPtr copy(vector<VRObjectPtr> children);

        void sendTypes();

        void handleWarning(string msg);
        void handleSelfmapRequest(string msg);
        void handleMapping(string mappingData);
        void handleTypeMapping(string mappingData);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        void gatherLeafs(VRObjectPtr parent, vector<pair<Node*, VRObjectPtr>>& leafs, vector<VRObjectPtr>& inconsistentCores);
        VRObjectPtr OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, Node* geoParent = 0);
        void wrapOSGLeaf(Node* node, VRObjectPtr parent);

        void handleNode(FieldContainerRecPtr& fcPtr, UInt32 nodeID, UInt32 coreID, map<UInt32,vector<UInt32>>& parentToChildren);
        void handleNodeCore(FieldContainerRecPtr& fcPtr, UInt32 remoteNodeID);

        void printRegistredContainers();
        void printSyncedContainers();
        void getAndBroadcastPoses();

        bool syncronizing = false;
        //void sync(string remoteUri);

        //Avatars
        void handlePoses(string poses);
        //void updateRemoteAvatarPose(string nodeName, PosePtr camPose);
        //void updateRemoteMousePose(string nodeName, PosePtr mousePose);
        map<string, PosePtr> remotesCameraPose;
        map<string, PosePtr> remotesMousePose;
        map<string, PosePtr> remotesFlystickPose;
        Pose oldCamPose;
        Pose oldMousePose;
        Pose oldFlystickPose;

        //Ownership
        vector<string> owned; //names of owned objects by this node
        void handleOwnershipMessage(string ownership);

        // avatar
        VRTransformPtr avatarHeadBeacon;
        VRTransformPtr avatarDeviceBeacon;
        void handleAvatar(string data);

        void handleNewConnect(string data);

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        void setDoWrapping(bool b);
        void setDoAvatars(bool b);

        void addRemote(string host, int port);

        void addRemoteMapping(UInt32 lID, UInt32 rID);
        void replaceContainerMapping(UInt32 ID1, UInt32 ID2);

        void startInterface(int port);
        void handleMessage(string msg);
        void update();
        void broadcast(string message);
        size_t getContainerCount();

        bool isRegistered(const UInt32& id);
        bool isSubContainer(const UInt32& id);
        bool isRemoteChange(const UInt32& id);
        bool isExternalContainer(const UInt32& id, UInt32& mask);

        void logSyncedContainer(UInt32 id);

        UInt32 getRemoteToLocalID(UInt32 id);
        UInt32 getLocalToRemoteID(UInt32 id);
        UInt32 getLocalType(UInt32 id);
        UInt32 getContainerMappedID(UInt32 id);
        VRObjectPtr getVRObject(UInt32 id);

        void analyseSubGraph();

        vector<string> getRemotes();
        PosePtr getRemoteCamPose(string remoteName);
        PosePtr getRemoteMousePose(string remoteName);
        PosePtr getRemoteFlystickPose(string remoteName);

        vector<string> getOwnedObjects(string nodeName);
        void requestOwnership(string objectName);
        void addOwnedObject(string objectName);

        void registerContainer(FieldContainer* c, UInt32 syncNodeID = -1);
        vector<UInt32> registerNode(Node* c); //returns all registered IDs
        void setAvatarBeacons(VRTransformPtr head, VRTransformPtr device);
        void addRemoteAvatar(VRTransformPtr head, VRTransformPtr device);

        map<FieldContainer*, vector<FieldContainer*>> getAllSubContainers(FieldContainer* node);

        void wrapOSG();

        string getConnectionLink();
        void setCallback(VRMessageCbPtr fkt);

        string getConnectionStatus();
};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
