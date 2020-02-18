#ifndef VRSyncNode_H_INCLUDED
#define VRSyncNode_H_INCLUDED

#include "VRTransform.h"
#include "core/networking/VRNetworkingFwd.h"
#include <OpenSG/OSGChangeList.h>

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
        VRSocketPtr socket;
        VRFunction<void*>* socketCb;
        VRUpdateCbPtr updateFkt;

        map<int, int> container; // local containers, sub-set of containers which need to be synced for collaboration
        vector<UInt32> syncedContainer; //Id's of container that got changes over sync (changed by remote). Needed to filter out sync changes from local Changelist to prevent cycles.
        map<string, VRSyncRemotePtr> remotes;
        map<int, int> syncNodeIDToLocalID;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        void handleChangeList(void* msg);
        vector<FieldContainer*> findContainer(string typeName); //deprecated
        vector<FieldContainer*> getTransformationContainer(ChangeList* cl); //deprecated
        //vector<OSG::Field

        string serialize(ChangeList* clist);
        void deserializeAndApply(string& data);

        void registerContainer(FieldContainer* c, int syncNodeID = -1);
        void registerNode(Node* c);

    public:
        VRSyncNode(string name = "syncNode");
        ~VRSyncNode();

        static VRSyncNodePtr create(string name = "None");
        VRSyncNodePtr ptr();

        void startInterface(int port);

        void addRemote(string host, int port, string name);

        void update();
        void printChangeList(ChangeList* cl);
        void broadcast(string message);

        void getContainer(); //deprecated
        //void getContainerFields();

        string printContainer(vector<FieldContainer*> container); //deprecated

};

OSG_END_NAMESPACE;

#endif // VRSyncNode_H_INCLUDED
