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
        string serverUri;
        bool doWrapping = true;

        VRTransformPtr avatarHeadTransform;
        VRTransformPtr avatarDeviceTransform;
        VRTransformPtr avatarDeviceAnchor;

        VRMessageCbPtr onEvent;
        VRSyncChangelistPtr changelist;

        map<UInt32, UInt32> container; // local containers, sub-set of containers which need to be synced for collaboration
        map<UInt32, UInt32> externalContainer; // local external containers, key is container ID, value is change mask to use
        map<string, VRSyncConnectionPtr> remotes;
        map<UInt32, VRObjectWeakPtr> nodeToVRObject;
        UInt32 getRegisteredContainerID(UInt32 syncID);
        UInt32 getRegisteredSyncID(UInt32 fieldContainerID);
        bool isRegisteredRemote(const UInt32& syncID);
        void getAllSubContainersRec(FieldContainer* node, FieldContainer* parent, map<FieldContainer*, vector<FieldContainer*>>& res);

        VRObjectPtr copy(vector<VRObjectPtr> children) override;

        void sendTypes(string remoteID);

        void handleWarning(string msg, string rID);
        void handleSelfmapRequest(string msg, string rID);
        void handleMapping(string mappingData, string rID);
        void handleTypeMapping(string mappingData, string rID);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        void gatherLeafs(VRObjectPtr parent, vector<pair<Node*, VRObjectPtr>>& leafs, vector<VRObjectPtr>& inconsistentCores);
        VRObjectPtr OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, Node* geoParent = 0);
        void wrapOSGLeaf(Node* node, VRObjectPtr parent);

        void handleNode(FieldContainerRecPtr& fcPtr, UInt32 nodeID, UInt32 coreID, map<UInt32,vector<UInt32>>& parentToChildren);
        void handleNodeCore(FieldContainerRecPtr& fcPtr, UInt32 remoteNodeID);

        void printRegistredContainers();

        //Ownership
        vector<string> owned; //names of owned objects by this node
        void handleOwnershipMessage(string ownership, string rID);

        void handleNewConnect(string data);
        void accTCPConnection(string msg, string rID);
        void reqInitState(string rID);

        UInt32 getNodeID(VRObjectPtr t);
        UInt32 getTransformID(VRTransformPtr t);
        void handleAvatar(string data, string rID);
        void updateAvatar(string data, string rID);

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        string setTCPClient(VRTCPClientPtr cli);
        string addTCPClient(VRTCPClientPtr cli);

        void setDoWrapping(bool b);
        void setDoAvatars(bool b);

        void addRemote(string host, int port);
        VRSyncConnectionPtr getRemote(string rID);

        void replaceContainerMapping(UInt32 ID1, UInt32 ID2, string rID);

        void startInterface(int port);
        string handleMessage(string msg, string rID);
        void update();
        void broadcast(string message);
        size_t getContainerCount();

        bool isRegistered(const UInt32& id);
        bool isSubContainer(const UInt32& id);
        bool isExternalContainer(const UInt32& id, UInt32& mask);

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
        void addExternalContainer(UInt32 id, UInt32 mask);
        vector<UInt32> registerNode(Node* c); //returns all registered IDs
        void setAvatarBeacons(VRTransformPtr headTransform, VRTransformPtr devTransform, VRTransformPtr devAnchor);
        void addRemoteAvatar(string remoteID, VRTransformPtr headTransform, VRTransformPtr devTransform, VRTransformPtr devAnchor);

        map<FieldContainer*, vector<FieldContainer*>> getAllSubContainers(FieldContainer* node);
        map<UInt32, VRObjectWeakPtr> getMappedFCs();

        void wrapOSG();

        string getConnectionLink();
        void setCallback(VRMessageCbPtr fkt);

        string getConnectionStatus();
};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
