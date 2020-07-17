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
//        VRSyncNodeFieldContainerMapper mapper;
        typedef unsigned char BYTE;
        VRSocketPtr socket;
        VRFunction<void*>* socketCb;
        VRUpdateCbPtr updateFkt;
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        vector<UInt32> createdNodes; //IDs of the currently created nodes/children
        vector<FieldContainerRecPtr> justCreated; //IDs of the currently created nodes/children

        VRSyncChangelistPtr changelist;

        map<UInt32, UInt32> container; // local containers, sub-set of containers which need to be synced for collaboration
        //vector<UInt32> cores; //lists IDs of nodecores
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        map<string, VRSyncConnectionPtr> remotes;
        map<UInt32, UInt32> remoteToLocalID;
        map<UInt32, UInt32> localToRemoteID;
        map<UInt32, UInt32> remoteCoreToLocalNode;
        map<UInt32, VRObjectWeakPtr> nodeToVRObject;
        UInt32 getRegisteredContainerID(UInt32 syncID);
        UInt32 getRegisteredSyncID(UInt32 fieldContainerID);
        bool isRegisteredRemote(const UInt32& syncID);
        vector<UInt32> getFCChildren(FieldContainer* fcPtr, BitVector fieldMask);
        void getAllSubContainersRec(FieldContainer* node, FieldContainer* parent, map<FieldContainer*, vector<FieldContainer*>>& res);
        UInt32 findParent(map<UInt32,vector<UInt32>>& parentToChildren, UInt32 remoteNodeID);

        VRObjectPtr copy(vector<VRObjectPtr> children);
        map<string, vector<string>> ownershipNodeToObject;

        void handleMessage(void* msg);
        void handleMapping(string mappingData);
        void handlePoses(string poses);
        void handleOwnership(string ownership);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        void filterFieldMask(FieldContainer* fc, SerialEntry& sentry);
        void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, UInt32 syncNodeID);

        void handleChildrenChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren);
        void handleCoreChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry);
        void handleGenericChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<BYTE>>& fcData);
        FieldContainerRecPtr getOrCreate(UInt32& id, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren);
        void printDeserializedData(vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<BYTE>>& fcData);
        void handleRemoteEntries(vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<BYTE>>& fcData);
        void deserializeEntries(string& data, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<BYTE>>& fcData);
        void deserializeAndApply(string& data);
        void deserializeChildrenData(vector<BYTE>& childrenData, map<UInt32,vector<UInt32>>& parentToChildren);

        void gatherLeafs(VRObjectPtr parent, vector<pair<Node*, VRObjectPtr>>& leafs, vector<VRObjectPtr>& inconsistentCores);
        VRObjectPtr OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, Node* geoParent = 0);
        void wrapOSGLeaf(Node* node, VRObjectPtr parent);
        void wrapOSG();

        void handleNode(FieldContainerRecPtr& fcPtr, UInt32 nodeID, UInt32 coreID, map<UInt32,vector<UInt32>>& parentToChildren);
        void handleNodeCore(FieldContainerRecPtr& fcPtr, UInt32 remoteNodeID);

        void printRegistredContainers();
        void printSyncedContainers();
        void getAndBroadcastPoses();

        bool syncronizing = false;
        void sync(string remoteUri);

        void updateRemoteAvatarPose(string nodeName, PosePtr camPose);
        void updateRemoteMousePose(string nodeName, PosePtr mousePose);
        map<string, PosePtr> remotesCameraPose;
        map<string, PosePtr> remotesMousePose;
        Pose oldCamPose;
        Pose oldMousePose;

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        void startInterface(int port);

        void addRemote(string host, int port, string name);

        void update();
        void broadcast(string message);
        size_t getContainerCount();

        bool isRegistered(const UInt32& id);
        bool isSubContainer(const UInt32& id);
        bool isRemoteChange(const UInt32& id);

        UInt32 getRemoteToLocalID(UInt32 id);

        void analyseSubGraph();

        PosePtr getRemoteCamPose(string remoteName);
        PosePtr getRemoteMousePose(string remoteName);

        vector<string> getOwnedObjects(string nodeName);

        void registerContainer(FieldContainer* c, UInt32 syncNodeID = -1);
        vector<UInt32> registerNode(Node* c); //returns all registered IDs

        map<FieldContainer*, vector<FieldContainer*>> getAllSubContainers(FieldContainer* node);
        string serialize(ChangeList* clist);
};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
