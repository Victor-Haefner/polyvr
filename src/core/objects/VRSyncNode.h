#ifndef VRSyncNode_H_INCLUDED
#define VRSyncNode_H_INCLUDED

#include "VRTransform.h"
#include "core/networking/VRNetworkingFwd.h"
#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGFieldContainerFactory.h>
//#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGContainerIdMapper.h>

class OSGChangeList;
struct SerialEntry;

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRSyncRemote);

class VRSyncRemote {//: public VRName {
    private:
        map<int, int> mapping; // <remote container ID, local container ID>
        string uri;

        VRWebSocketPtr socket;

    public:
        VRSyncRemote(string uri = "");
        ~VRSyncRemote();

        void connect();
        bool send(string message);
        static VRSyncRemotePtr create(string name = "None");
//        VRSyncRemotePtr ptr();
};

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

        map<int, int> container; // local containers, sub-set of containers which need to be synced for collaboration
        //vector<int> cores; //lists IDs of nodecores
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        map<string, VRSyncRemotePtr> remotes;
        map<int, int> remoteToLocalID;
        map<UInt32, UInt32> remoteCoreToLocalNode;
        UInt32 getRegisteredContainerID(int syncID);
        int getRegisteredSyncID(UInt32 fieldContainerID);
        bool isRegisteredRemote(const UInt32& syncID);
        vector<int> getFCChildren(FieldContainer* fcPtr, BitVector fieldMask);
        void getAllSubContainersRec(FieldContainer* node, FieldContainer* parent, map<FieldContainer*, vector<FieldContainer*>>& res);
        map<FieldContainer*, vector<FieldContainer*>> getAllSubContainers(FieldContainer* node);
        int findParent(map<int,vector<int>>& parentToChildren, int remoteNodeID);

        VRObjectPtr copy(vector<VRObjectPtr> children);

        string copySceneState();
        void handleChangeList(void* msg);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        void filterFieldMask(FieldContainer* fc, SerialEntry& sentry);
        void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID);
        string serialize(ChangeList* clist);

        void handleChildrenChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<int, vector<int>>& parentToChildren);
        void handleCoreChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry);
        void handleGenericChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<int, vector<BYTE>>& fcData);
        FieldContainerRecPtr getOrCreate(UInt32& id, SerialEntry& sentry, map<int, vector<int>>& parentToChildren);
        void printDeserializedData(vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData);
        void handleRemoteEntries(vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData);
        void deserializeEntries(string& data, vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData);
        void deserializeAndApply(string& data);
        void deserializeChildrenData(vector<BYTE>& childrenData, map<int,vector<int>>& parentToChildren);

        void registerContainer(FieldContainer* c, int syncNodeID = -1);
        vector<int> registerNode(Node* c); //returns all registered IDs

        void handleNode(FieldContainerRecPtr& fcPtr, UInt32 nodeID, UInt32 coreID, map<int,vector<int>>& parentToChildren);
        void handleNodeCore(FieldContainerRecPtr& fcPtr, UInt32 remoteNodeID);

        bool isRemoteChange(const UInt32& id);
        bool isRegistered(const UInt32& id);
        bool isSubContainer(const UInt32& id);

        void printRegistredContainers();
        void printSyncedContainers();
        void printChangeList(OSGChangeList* cl);
        void broadcastChangeList(OSGChangeList* cl, bool doDelete = false);
        OSGChangeList* getFilteredChangeList();

        bool syncronizing = false;
        void sync(string remoteUri);

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        void startInterface(int port);

        void addRemote(string host, int port, string name);

        void update();
        void broadcast(string message);

        UInt32 getRemoteToLocalID(UInt32 id);
};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED