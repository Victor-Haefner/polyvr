#include "VRSyncNode.h"
#include "VRLight.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/utils/VRStorage_template.h"
#include "core/networking/VRSocket.h"
#include "core/networking/VRWebSocket.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGNodeCore.h>
#include <OpenSG/OSGTransformBase.h>
#include "core/objects/OSGTransform.h"

#include <OpenSG/OSGThreadManager.h>

//BUGS:
/*
Known bugs:
    - syncedContainer seem not get get empty (keeps filling with same id)
    - in PolyVR: when syncNodes are not initialized on_scene_load but by manually triggering the script - the program will crash
    - syncNodes need to be translated on initialization else no Transform Node will be created to track (PolyVR optimisation initialises with Group Node type; Transform Node only creates after a Transformation)

*/

//TODO:
/*
    - create (Node/Child) change handling and applying on remote SyncNode
    - copy Changes from state for initialization of new remote SyncNode from master's State
    - remove Changes (derefferencing?)
*/

using namespace OSG;

template<> string typeName(const VRSyncNode& o) { return "SyncNode"; }

ThreadRefPtr applicationThread;

void VRSyncNode::printChangeList(ChangeList* cl) {
    //if (cl->size() == 0) cout << "VRSyncNode::printChangeList: empty Changelist." << endl;

    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    stringstream changedContainers;
    int j = 0;
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        if (j == 0) cout << "VRSyncNode::printChangeList " << name << endl;
        ContainerChangeEntry* entry = *it;
        const FieldFlags* fieldFlags = entry->pFieldFlags;
        BitVector whichField = entry->whichField;
        UInt32 id = entry->uiContainerId;

        // ----- print info ---- //
        string type = "";
        if (factory->getContainer(id) != nullptr){
            type += factory->getContainer(id)->getTypeName();
        }
        cout << "whichField " << whichField << ", uiContainerId: " << id << " containerType: " << type << endl;
        j++;
    }
}

VRSyncNode::VRSyncNode(string name) : VRTransform(name) {
    type = "SyncNode";
    applicationThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());

    // TODO: get all container and their ID in this VRTransform
    NodeMTRefPtr node = getNode()->node;
    registerNode(node);

    //OSGTransformPtr pt = getOSGTransformPtr();
    //registerContainer(pt->trans); //transform

    //cout << "VRSyncNode::VRSyncNode " << name << "  " << node->getTypeName() << ":" << node->getId() << ", " << core->getTypeName() << ":" << core->getId() << " ! " << pt->trans->getTypeName() << ":" << pt->trans->getId() << endl;

	updateFkt = VRUpdateCb::create("SyncNode update", bind(&VRSyncNode::update, this));
	VRScene::getCurrent()->addUpdateFkt(updateFkt, 100000);
}

VRSyncNode::~VRSyncNode() {
    cout << "VRSyncNode::~VRSyncNode " << name << endl;
}

VRSyncNodePtr VRSyncNode::ptr() { return static_pointer_cast<VRSyncNode>( shared_from_this() ); }
VRSyncNodePtr VRSyncNode::create(string name) { return VRSyncNodePtr(new VRSyncNode(name) ); }

class OSGChangeList : public ChangeList {
    public:
        ~OSGChangeList() {};

        void addCreate(ContainerChangeEntry* entry) {
            ContainerChangeEntry* pEntry = getNewCreatedEntry();
            pEntry->uiEntryDesc   = entry->uiEntryDesc;
            pEntry->uiContainerId = entry->uiContainerId;
            pEntry->whichField    = entry->whichField;
            if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                pEntry->whichField |= *entry->bvUncommittedChanges;
            pEntry->pList         = this;
        }

//        void addCreate(UInt32 uiContainerId, BitVector bFlags) {
//            ContainerChangeEntry* pEntry = getNewCreatedEntry();
//            pEntry->uiEntryDesc   = ContainerChangeEntry::Create;
//            pEntry->uiContainerId = uiContainerId;
//            pEntry->whichField    = bFlags;
////            if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
////                pEntry->whichField |= *entry->bvUncommittedChanges;
//            pEntry->pList         = this;
//            cout << "ChangeList addCreate" << endl;
//        }

        void addChange(ContainerChangeEntry* entry) {
                if (entry->uiEntryDesc == ContainerChangeEntry::AddReference   ||
                    entry->uiEntryDesc == ContainerChangeEntry::SubReference   ||
                    entry->uiEntryDesc == ContainerChangeEntry::DepSubReference ) {
                    ContainerChangeEntry *pEntry = getNewEntry();
                    pEntry->uiEntryDesc   = entry->uiEntryDesc;
                    pEntry->uiContainerId = entry->uiContainerId;
                    pEntry->pList         = this;
                } else if(entry->uiEntryDesc == ContainerChangeEntry::Change) {
                    ContainerChangeEntry *pEntry = getNewEntry();
                    pEntry->uiEntryDesc   = ContainerChangeEntry::Change;
                    pEntry->pFieldFlags   = entry->pFieldFlags;
                    pEntry->uiContainerId = entry->uiContainerId;
                    pEntry->whichField    = entry->whichField;
                    if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                        pEntry->whichField |= *entry->bvUncommittedChanges;
                    pEntry->pList         = this;
                }
            cout << "ChangeList addChange" << endl;
        }
};

typedef unsigned char BYTE;

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(BYTE c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(BYTE const* buf, unsigned int bufLen) {
  std::string ret;
  int i = 0;
  int j = 0;
  BYTE char_array_3[3];
  BYTE char_array_4[4];

  while (bufLen--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}

std::vector<BYTE> base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  std::vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}

struct SerialEntry {
    int localId = 0;
    BitVector fieldMask;
    int len = 0;
    int syncNodeID = -1;
    int uiEntryDesc = -1;
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) {
            cout << "ourBinaryDataHandler -> read() data.size" << data.size() << " size " << size << endl;
            memcpy(src, &data[0], min((size_t)size, data.size())); //read data from handler into src (sentry.fieldMask)
            //memcpy(src, &data[0], data.size());
        }

        void write(MemoryHandle src, UInt32 size) {
            data.insert(data.end(), src, src + size);
        }

        vector<BYTE> data;
};

void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID) {
    UInt32 id = entry->uiContainerId;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainer* fcPtr = factory->getContainer(id);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = id;
        sentry.fieldMask = entry->whichField;
        sentry.syncNodeID = syncNodeID;
        sentry.uiEntryDesc = entry->uiEntryDesc;

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        data.insert(data.end(), (BYTE*)&sentry, (BYTE*)&sentry + sizeof(SerialEntry)); //v.insert(v.end(), data.begin(), data.end())
        data.insert(data.end(), handler.data.begin(), handler.data.end());

        cout << "serialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " | encoded: " << data.size() << endl;
    }
}

string VRSyncNode::serialize(ChangeList* clist) {
    cout << "> > >  " << name << " VRSyncNode::serialize()" << endl;
    vector<BYTE> data;
    for (auto it = clist->begin(); it != clist->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (entry->uiEntryDesc != ContainerChangeEntry::Change) continue;
        serialize_entry(entry, data, container[id]);
    }
    cout << "            / " << name << " / VRSyncNode::serialize()" <<"  < < <" << endl;
    return base64_encode(&data[0], data.size());
}

void VRSyncNode::deserializeAndApply(string& data) {
    cout << "> > >  " << name << " VRSyncNode::deserializeAndApply()" << endl;
    vector<BYTE> vec = base64_decode(data);
    int pos = 0;
    while (pos < vec.size()) {
        //cout << " !!! search for sentry at " << pos << "/" << vec.size() << endl;
        SerialEntry sentry = *((SerialEntry*)&vec[pos]);
        cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << endl;
        pos += sizeof(SerialEntry);
        vector<BYTE> FCdata;
        FCdata.insert(FCdata.end(), vec.begin()+pos, vec.begin()+pos+sentry.len);
        pos += sentry.len;

        // TODO:
        //  - get corresponding ID to sentry.localId
        UInt32 id = -1;//3021;
        if (sentry.syncNodeID != -1) {
            for (auto c : container) {
                if (c.second == sentry.syncNodeID) {
                    id = c.first;
                    remoteToLocalID[sentry.syncNodeID] = id; // TODO: write in mapping!
                    break;
                }
            }
        }
        if (id == -1) continue;

        syncedContainer.push_back(id);

        //  - get fieldcontainer using correct ID
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        FieldContainer* fcPtr = factory->getContainer(id);
        cout << name << " apply data to " << fcPtr->getTypeName() << " (" << fcPtr->getTypeId() << ")" << endl;
        string type = fcPtr->getTypeName();
        ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer (use connection instead of handler, see OSGRemoteaspect.cpp (receiveSync))
        handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata
        if (type == "Node") {
            //check for children changes
            if(sentry.fieldMask & Node::ChildrenFieldMask){
                cout << "we've got a change in children here " << sentry.syncNodeID << " , " << sentry.uiEntryDesc << ", CreateDesc(1) " << ContainerChangeEntry::Create << endl;
                //have we got a corresponding child id in local container of this SyncNode?
                if (sentry.uiEntryDesc = ContainerChangeEntry::Create){
                     createChild();
                    //Node::addChild() //child pointer?
                }
            }
        }
        fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read
    }
    //Thread::getCurrentChangeList()->commitChanges(ChangedOrigin::Sync);
    //Thread::getCurrentChangeList()->commitChangesAndClear(ChangedOrigin::Sync);

    //check if all cores are still there
//    cout << "registered container" << endl;
//    for (auto c : container) {
//        cout << c.first << endl;
//    }
//    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
//    for (auto c : container) {
//        UInt32 id = c.first;
//        if (!factory->getContainer(id)) cout << "no FC with id " << id << endl; continue;
//        FieldContainer* fc = factory->getContainer(id);
//        string type = fc->getTypeName();
//        Node* node = dynamic_cast<Node*>(fc);
//        if(type == "Node") {
//            cout << "Node " << c.first << " " << type << endl;
//            if(node->getCore()){
//                cout << "Core " << node->getCore() << endl;
//            } else cout << "no Core" << endl;
//        }
//    }
    //cout << "!!!!! registered 2 field container " << factory->getContainer(node1->getId())->getTypeName() << " and " << factory->getContainer(group->getId())->getTypeName() << endl;


    cout << "            / " << name << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;
}

//create a node structure for create child
//when a geometry is added to a scene Graph node, the following container are registered and need to be created on remote node: Node, Group, Node, Geometry, Node, Transform, Node, Geometry
//TODO: shorten with helper functions
void VRSyncNode::createChild(){
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();

    //Node
    NodeRefPtr n1 = Node::create();
    Node* node1 = n1.get();
    //Group
    GroupRefPtr group = Group::create();
    n1->setCore(group);


    registerNode(node1);
    syncedContainer.push_back(node1->getId());
    syncedContainer.push_back(group->getId());
//    factory->registerContainer(node1); //TODO: check if factory-> registerContainer is really needed here
//    factory->registerContainer(group);

    //Node
    NodeRefPtr n2 = Node::create();
    Node* node2 = n2.get();
    //Geometry //GeometryMTRecPtr geo = Geometry::create();
    GeometryMTRecPtr g1 = makeTorusGeo(0.5, 1.0, 16, 8); //VRTorus::make();
    n2->setCore(g1);

    registerNode(node2);
    syncedContainer.push_back(node2->getId());
    syncedContainer.push_back(g1->getId());
//    factory->registerContainer(node2);
//    factory->registerContainer(g1);


    //Node
    NodeRefPtr n3 = Node::create();
    Node* node3 = n3.get();
    //Transform
    Matrix m;
    m.setTransform(Vec3f(0,  0, 0));
    TransformRecPtr trans  = Transform::create(); //node core
    trans->setMatrix(m);
    n3->setCore(trans);

    registerNode(node3);
    syncedContainer.push_back(node3->getId());
    syncedContainer.push_back(trans->getId());
//    factory->registerContainer(node3);
//    factory->registerContainer(trans);


    //Node
    NodeRefPtr n4 = Node::create();
    Node* node4 = n4.get();
    //Geometry
    GeometryMTRecPtr g2 = makeTorusGeo(0.5, 1.0, 16, 8);
    n4->setCore(g2);

    registerNode(node4);
    syncedContainer.push_back(node4->getId());
    syncedContainer.push_back(g2->getId());
//    factory->registerContainer(node4);
//    factory->registerContainer(g2);
}



//update this SyncNode
void VRSyncNode::update() {
    // go through all changes, gather changes where the container is known (in containers)
    ChangeList* cl = applicationThread->getChangeList();
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return;
    cout << "> > >  " << name << " VRSyncNode::update()" << endl;
    //printChangeList(cl);

    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    int j = 0;

    // create local changelist with changes of containers of the subtree of this sync node :D
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        //if (entry->uiEntryDesc == ContainerChangeEntry::Create) cout << ">>>>>>>>ContainerChangeEntry::Create" << endl;
        if (entry->uiEntryDesc != ContainerChangeEntry::Change) continue; // TODO: only for debugging!
        UInt32 id = entry->uiContainerId;
        if (container.count(id)) {
            if (::find(syncedContainer.begin(), syncedContainer.end(), id) == syncedContainer.end()) { // check to avoid adding a change induced by remote sync
                //cout << " change ? " << id << "  " << entry->whichField << "  " << Node::ChildrenFieldMask << endl;
                //cout << " change ? " << id << "  " << *entry->bvUncommittedChanges << "  " << Node::ChildrenFieldMask << endl;
                cout << "localChanges->addChange " << endl;
                localChanges->addChange(entry);
            }
        }
    }

    if (localChanges->getNumChanged() == 0){
        cout << "            / " << name << " VRSyncNode::update()" << " < < <" << endl;
        return;
    }

    // check for addChild changes
    for (auto it = localChanges->begin(); it != localChanges->end(); ++it) {
        cout << "update child changes" << endl;
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        FieldContainer* fct = factory->getContainer(id);
        if (fct){
            string type = fct->getTypeName();

            if (type == "Node" && entry->whichField & Node::ChildrenFieldMask) { //if the children filed mask of node has changed, we check if a new child was added
                Node* node = dynamic_cast<Node*>(fct);
                for (int i=0; i<node->getNChildren(); i++) {
                    Node* child = node->getChild(i);
                    if (!container.count(child->getId())) { //check if it is an unregistered child
                        registerNode(child);
                        //localChanges->addCreate(child->getId(), TypeTraits<BitVector>::BitsClear); // TODO: add created for every new registered container?
                    }
                }
            }

            if (type == "Node" && entry->whichField & Node::CoreFieldMask) { // core change of known node
                //cout << "  node core changed!" << endl;
                Node* node = dynamic_cast<Node*>(fct);
                //registerContainer(node->getCore()); // TODO: add created?
                registerContainer(node->getCore(), container.size()); // TODO: just for testing!!
            }
        }
    }

    // check for created nodes
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (container.count(id)) localChanges->addCreate(entry);
    }

    cout << "print local changes: " << endl;
    printChangeList(localChanges);

    // serialize changes in new change list (check OSGConnection for serialization Implementation)
    string data = serialize(localChanges);
    delete localChanges;

    // send over websocket to remote
    for (auto& remote : remotes) {
        remote.second->send(data);
        //cout << name << " sending " << data  << endl;
    }

    syncedContainer.clear();

    //DEBUG: print registered container
    cout << "print registered container: " << endl;
    for (auto c : container){
        UInt32 id = c.first;
        cout << id << " syncNodeID " << c.second ;
        if (factory->getContainer(id)){
            cout << " " << factory->getContainer(id)->getTypeName();
        }
        cout << endl;
    }

    cout << "            / " << name << " VRSyncNode::update()" << "  < < < " << endl;
}

void VRSyncNode::registerContainer(FieldContainer* c, int syncNodeID) {
    cout << " VRSyncNode::registerContainer " << getName() << " container: " << c->getTypeName() << " at fieldContainerId: " << c->getId() << endl;
    container[c->getId()] = syncNodeID;
}

void VRSyncNode::registerNode(Node* node) {
    NodeCoreMTRefPtr core = node->getCore();
    registerContainer(node, container.size());
    registerContainer(core, container.size());
    for (int i=0; i<node->getNChildren(); i++) registerNode(node->getChild(i));
}

VRObjectPtr VRSyncNode::copy(vector<VRObjectPtr> children) {
    return 0;
}

void VRSyncNode::startInterface(int port) {
    socket = VRSceneManager::get()->getSocket(port);
    socketCb = new VRHTTP_cb( "VRSyncNode callback", bind(&VRSyncNode::handleChangeList, this, _1) );
    socket->setHTTPCallback(socketCb);
    socket->setType("http receive");
}

string asUri(string host, int port, string name) {
    return "ws://" + host + ":" + to_string(port) + "/" + name;
}

//Add remote Nodes to sync with
void VRSyncNode::addRemote(string host, int port, string name) {
    string uri = asUri(host, port, name);
    remotes[uri] = VRSyncRemote::create(uri);

//    ChangeList* cl = (OSGChangeList*)ChangeList::create();
//    cl->fillFromCurrentState(0,1);
//    printChangeList(cl);
}

void VRSyncNode::handleChangeList(void* _args) {
    HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) cout << "AAAARGH" << endl;

    int client = args->ws_id;
    string msg = args->ws_data;

    //cout << "GOT CHANGES!! " << endl;
    //cout << name << ", received msg: "  << msg << endl;//<< " container: ";

    deserializeAndApply(msg);
}

//broadcast message to all remote nodes
void VRSyncNode::broadcast(string message){
    for (auto& remote : remotes) {
        if (!remote.second->send(message)) {
            cout << "Failed to send message to remote." << endl;
        }
    }
}

//deprecated
//prints all container
void VRSyncNode::getContainer(){
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    //auto containerStore = factory->getFieldContainerStore();

    for( auto it = factory->beginStore(); it != factory->endStore(); ++it) {
        AspectStore* aspect = it->second;
        FieldContainer* container = aspect->getPtr();

        cout << "container Id: " << factory->findContainer(container) << " | size: " << container->getContainerSize();
        cout << " | #Fields: " << container->getNumFields() << " | type name: " << container->getTypeName() << endl;

    }

}

//deprecated
//lists all container with typeName
vector<FieldContainer*> VRSyncNode::findContainer(string typeName){
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    vector<FieldContainer*> res;
    for( auto it = factory->beginStore(); it != factory->endStore(); ++it){
        AspectStore* aspect = it->second;
        FieldContainer* container = aspect->getPtr();
        if (container->getTypeName() == typeName){
            res.push_back(container);
        }
    }
    return res;
}

//deprecated
//returns container of type Transformation from changelist
vector<FieldContainer*> VRSyncNode::getTransformationContainer(ChangeList* cl){
    vector<FieldContainer*> res;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    for( auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        FieldContainer* container = factory->getContainer(id);
        if (container != nullptr){
            if(strcmp(container->getTypeName(), "Transform") == 0){
                res.push_back(container);
            }
        }
    }
    return res;
}

//returns container infos as a string
string VRSyncNode::printContainer(vector<FieldContainer*> container){
    std::stringstream res;
    for( auto c : container) {
        UInt32 id = c->getId();
        string typeName = c->getTypeName();
        UInt32 typeID = c->getTypeId();
        UInt16 groupID = c->getGroupId();
        UInt32 containerSize = c->getContainerSize();
        UInt32 numFields = c->getNumFields();
        const FieldFlags* fieldFlags = c->getFieldFlags();


        vector<string> fieldNames;
        for(int i = 0; i<numFields; ++i){
            GetFieldHandlePtr fh = c->getField(i);
            if(fh != nullptr){
                cout << fh->getName() << endl;
            }
        }
        //cout << "container id: " << c->getId() << endl;
        res << "container " << id << " " << typeName << " | typeID " << typeID  << " | groupID " << groupID << " | size " << containerSize << " | numFields " << numFields << endl;
    }
    return res.str();
}

// ------------------- VRSyncRemote


VRSyncRemote::VRSyncRemote(string uri) : uri(uri) {
    socket = VRWebSocket::create("sync node ws");
    //cout << "VRSyncRemote::VRSyncRemote " << uri << endl;
    if (uri != "") connect();
    //map IDs
}

VRSyncRemote::~VRSyncRemote() { cout << "~VRSyncRemote::VRSyncRemote" << endl; }
VRSyncRemotePtr VRSyncRemote::create(string name) { return VRSyncRemotePtr( new VRSyncRemote(name) ); }

void VRSyncRemote::connect() {
    //cout << "VRSyncRemote, try connecting to " << uri << endl;
    bool result = socket->open(uri);
    if (!result) cout << "VRSyncRemote, Failed to open websocket to " << uri << endl;
    //else cout << "VRSyncRemote, connected to " << uri << endl;
}

bool VRSyncRemote::send(string message){
    if (!socket->sendMessage(message)) return 0;
    return 1;
}
