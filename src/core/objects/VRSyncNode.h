#ifndef VRSyncNode_H_INCLUDED
#define VRSyncNode_H_INCLUDED

#include "VRTransform.h"
#include "core/networking/VRNetworkingFwd.h"
#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGFieldContainerFactory.h>

class OSGChangeList;

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
        typedef unsigned char BYTE;
        VRSocketPtr socket;
        VRFunction<void*>* socketCb;
        VRUpdateCbPtr updateFkt;
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        vector<UInt32> createdNodes; //IDs of the currently created nodes/children

        map<int, int> container; // local containers, sub-set of containers which need to be synced for collaboration
        //vector<int> cores; //lists IDs of nodecores
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        map<string, VRSyncRemotePtr> remotes;
        map<int, int> remoteToLocalID;
        UInt32 getRegisteredContainerID(int syncID);
        int getRegisteredSyncID(UInt32 fieldContainerID);
        UInt32 getLocalId(UInt32 remoteID, int syncID);
        bool isRegistered(int syncID);

        VRObjectPtr copy(vector<VRObjectPtr> children);

        string copySceneState();
        void handleChangeList(void* msg);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID);
        string serialize(ChangeList* clist);
        void deserializeAndApply(string& data);
        void deserializeChildrenData(vector<BYTE>& childrenData, UInt32 fcID, map<int,int>& childToParent);

        void registerContainer(FieldContainer* c, int syncNodeID = -1);
        vector<int> registerNode(Node* c); //returns all registered IDs

        void createNode(FieldContainerRecPtr& fcPtr, int syncNodeID, map<int,int>& childToParent);
        void createNodeCore(FieldContainerRecPtr& fcPtr, int syncNodeID, map<int,int>& childToParent);

        bool isRemoteChange(const UInt32& id);
        bool isRegistred(const UInt32& id);
        bool isSubContainer(const UInt32& id);

        void printRegistredContainers();
        void printSyncedContainers();
        void printChangeList(OSGChangeList* cl);
        void broadcastChangeList(OSGChangeList* cl, bool doDelete = false);
        OSGChangeList* getFilteredChangeList();

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        void startInterface(int port);

        void addRemote(string host, int port, string name);

        void update();
        void broadcast(string message);


};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
