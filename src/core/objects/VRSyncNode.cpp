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
    if (cl->getNumChanged() == 0 && cl->getNumCreated() == 0) cout << "no changes " << endl;
    stringstream changedContainers;
    int j = 0;
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        if (j == 0) cout << "VRSyncNode::printChangeList " << name << " changed: " << cl->getNumChanged() << " created: " << cl->getNumCreated() << endl;
        ContainerChangeEntry* entry = *it;
        const FieldFlags* fieldFlags = entry->pFieldFlags;
        BitVector whichField = entry->whichField;
        UInt32 id = entry->uiContainerId;

        // ----- print info ---- //
        string type = "";
        if (factory->getContainer(id)){
            type += factory->getContainer(id)->getTypeName();
        }
        cout << "uiContainerId: " << id;
        string changeType = "";
        switch (entry->uiEntryDesc) {
            case ContainerChangeEntry::Change:
                changeType = " Change            ";
                break;
            case ContainerChangeEntry::AddField:
                changeType = " addField/SubField ";
                break;
            case ContainerChangeEntry::AddReference:
                changeType = " AddReference      ";
                break;
            case ContainerChangeEntry::Create:
                changeType = " Create            ";
                break;
            case ContainerChangeEntry::DepSubReference:
                changeType = " DepSubReference   ";
                break;
            case ContainerChangeEntry::SubReference:
                changeType = " SubReference      ";
                break;
            default:
                changeType = " none              ";;
        }
        cout << " " << changeType << " container: " << type<< endl;
        j++;
    }
}

VRSyncNode::VRSyncNode(string name) : VRTransform(name) {
    type = "SyncNode";
    applicationThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());

    NodeMTRefPtr node = getNode()->node;
    registerNode(node);

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

        void addChange(ContainerChangeEntry* entry) {
                if (entry->uiEntryDesc == ContainerChangeEntry::AddReference   ||
                    entry->uiEntryDesc == ContainerChangeEntry::SubReference   ||
                    entry->uiEntryDesc == ContainerChangeEntry::DepSubReference) {
                    ContainerChangeEntry *pEntry = getNewEntry();
                    pEntry->uiEntryDesc   = entry->uiEntryDesc;
                    pEntry->uiContainerId = entry->uiContainerId;
                    pEntry->pList         = this;
                } else if(entry->uiEntryDesc == ContainerChangeEntry::Change||
                    entry->uiEntryDesc == ContainerChangeEntry::Create) {
                    ContainerChangeEntry *pEntry = getNewEntry();
                    pEntry->uiEntryDesc   = entry->uiEntryDesc; //ContainerChangeEntry::Change; //TODO: check what I did here (workaround to get created entries into the changelist aswell)
                    pEntry->pFieldFlags   = entry->pFieldFlags;
                    pEntry->uiContainerId = entry->uiContainerId;
                    pEntry->whichField    = entry->whichField;
                    if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                        pEntry->whichField |= *entry->bvUncommittedChanges;
                    pEntry->pList         = this;
                }
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


void printNodeFieldMask(BitVector fieldmask) {
    string changeType = "";
    if (fieldmask & Node::AttachmentsFieldMask) changeType = " AttachmentsFieldMask";
    else if (fieldmask & Node::bInvLocalFieldMask) changeType = " bInvLocalFieldMask ";
    else if ( fieldmask & Node::bLocalFieldMask) changeType = " bLocalFieldMask      ";
    else if ( fieldmask & Node::ChangedCallbacksFieldMask) changeType = " ChangedCallbacksFieldMask";
    else if ( fieldmask & Node::ChildrenFieldMask) changeType = " ChildrenFieldMask   ";
    else if ( fieldmask & Node::ContainerIdMask) changeType = " ContainerIdMask ";
    else if ( fieldmask & Node::DeadContainerMask) changeType = " DeadContainerMask ";
    else if ( fieldmask & Node::NextFieldMask) changeType = " NextFieldMask ";
    else if ( fieldmask & Node::ParentFieldMask) changeType = " ParentFieldMask ";
    else if ( fieldmask & Node::SpinLockClearMask) changeType = " SpinLockClearMask ";
    else if ( fieldmask & Node::TravMaskFieldMask) changeType = " TravMaskFieldMask ";
    else if ( fieldmask & Node::VolumeFieldMask) changeType = " VolumeFieldMask ";
    else if ( fieldmask & Node::CoreFieldMask) changeType = " CoreFieldMask ";
    else changeType = " none ";

    cout << changeType << endl;
}

void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID, FieldContainerFactoryBase* factory) {
    UInt32 id = entry->uiContainerId;
    //FieldContainerFactoryBase* factory = FieldContainerFactory::the();
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
        printNodeFieldMask(sentry.fieldMask);
    }
}

string VRSyncNode::serialize(ChangeList* clist) {
    int counter = 0;
    cout << "> > >  " << name << " VRSyncNode::serialize()" << endl;
    vector<BYTE> data;
    for (auto it = clist->begin(); it != clist->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        //if (entry->uiEntryDesc != ContainerChangeEntry::Change) continue;
        serialize_entry(entry, data, container[id], factory);
        counter++;
    }
    cout << "serialized entries: " << counter << endl;
    cout << "            / " << name << " / VRSyncNode::serialize()" <<"  < < <" << endl;
    return base64_encode(&data[0], data.size());
}

void VRSyncNode::deserializeAndApply(string& data) {
    cout << "> > >  " << name << " VRSyncNode::deserializeAndApply()" << endl;
    vector<BYTE> vec = base64_decode(data);
    int pos = 0;
    int counter = 0;
    ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer (use connection instead of handler, see OSGRemoteaspect.cpp (receiveSync))
    Node* parent;

    //deserialize and collect change and create entries
    while (pos < vec.size()) {
        SerialEntry sentry = *((SerialEntry*)&vec[pos]);
        cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << endl;
        pos += sizeof(SerialEntry);
        vector<BYTE> FCdata;
        FCdata.insert(FCdata.end(), vec.begin()+pos, vec.begin()+pos+sentry.len);
        pos += sentry.len;
        counter++;

        // map remote id to local id if exist (otherwise id = -1)
        UInt32 id = -1;//3021;
        if (sentry.syncNodeID != -1) {
            for (auto c : container) {
                if (c.second == sentry.syncNodeID) {
                    id = c.first;
                    remoteToLocalID[sentry.syncNodeID] = id;
                    cout << "container for syncNodeID " << sentry.syncNodeID << " container id " << id << endl;
                    break;
                }
            }
        }

        //replace remote entry ID with local ID
        if (id != -1) sentry.localId = id; //TODO: if works integrate it into previous loop
        //else cout << "got sentry with id -1 !!!!!!" << endl;

        if (sentry.fieldMask & Node::ChildrenFieldMask) {
            cout << "Node::ChildrenFieldMask changed" << endl;
            //we got a node whose children changed
        }


        //parentsfieldmask changed?
        if (sentry.fieldMask & NodeCore::ParentsFieldMask) {
            cout << "NodeCore::ParentsFieldMask changed" << endl;
            //we got a node core whose parent changed
        }

        FieldContainer* fc = nullptr; // Field Container to apply changes to
//        if (id == -1) continue;
        // if fc does not exist we need to create a new fc = node
        //TODO: continue here with creating/addressing node cores
        if (sentry.uiEntryDesc == ContainerChangeEntry::Create && id == -1) { //if (id == -1) {
            FieldContainer* fc = createChild(parent); //creates node with a core and registers with syncNodeID in container
            //FieldContainer* fc = dynamic_cast<FieldContainer*>(node);
            //FieldContainer* fc = factory->getContainer(node->getId());
            if (!fc) cout << "no field container!!!! for node id " << endl;
            UInt32 fcId = factory->findContainer(fc);
            UInt32 fcId2 = fc->getId();
            string fcType = fc->getTypeName();
            //cout << "created fc !!!!!! " << fc->getTypeName() << " " << fc->getId() << endl;
            registerContainer(fc, sentry.syncNodeID);
            int coreSyncID = ++sentry.syncNodeID;
            cout << "nodeID " << sentry.syncNodeID << " coreSyncID " << coreSyncID << endl;
            Node* node = dynamic_cast<Node*>(fc);
            FieldContainer* coreFC = dynamic_cast<FieldContainer*>(node->getCore());
            registerContainer(coreFC, coreSyncID); //Assume: coreID = nodeID+1

            syncedContainer.push_back(node->getId()); //ignore changes of FC with this id
            syncedContainer.push_back(node->getCore()->getId());
        }
        //if (sentry.uiEntryDesc == ContainerChangeEntry::Change) {
        else {
            fc = factory->getContainer(id);
        }
        cout << "field container " << fc << endl;

        //did the children field mask change?
        if (sentry.fieldMask & Node::ChildrenFieldMask) {
            //set parent for next entry
            parent = dynamic_cast<Node*>(fc);
        }

        //fc = factory->getContainer(id);
        //  - get fieldcontainer using correct ID
        if(!fc) {cout << "no container found with id " << id << " syncNodeID " << sentry.syncNodeID << endl; continue;} //TODO: This is causing the WARNING: Action::recurse: core is NULL, aborting traversal.
        cout << name << " apply data to " << fc->getTypeName() << " (" << fc->getTypeId() << ")" << endl;

        string type = fc->getTypeName();
        handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata

        fc->copyFromBin(handler, sentry.fieldMask); //calls handler->read

        syncedContainer.push_back(id);
    }

    //Thread::getCurrentChangeList()->commitChanges(ChangedOrigin::Sync);
    //Thread::getCurrentChangeList()->commitChangesAndClear(ChangedOrigin::Sync);

    cout << "deserialized entries: " << counter << endl;
    cout << "            / " << name << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;
}

//create a node and core for created child change entry
FieldContainer* VRSyncNode::createChild(Node* parent){
    NodeRefPtr nodePtr = Node::create();
    GroupRefPtr group = Group::create();
    nodePtr->setCore(group);
    Node* node = nodePtr.get();
    parent->addChild(node);
    cout << "created node " << node->getId() << endl;
    FieldContainer* fc = factory->getContainer(node->getId());
    if (!fc) cout << "no fc found for node" << endl;
    else cout << "found fc " << fc->getTypeName() << " " << fc->getId() << endl;
    return fc;
}



//update this SyncNode
void VRSyncNode::update() {
    // go through all changes, gather changes where the container is known (in containers)
    ChangeList* cl = applicationThread->getChangeList();
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return;
    cout << "> > >  " << name << " VRSyncNode::update()" << endl;
    //printChangeList(cl);
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

    int j = 0;
    // create local changelist with changes of containers of the subtree of this sync node :D
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (container.count(id)) {
            if (::find(syncedContainer.begin(), syncedContainer.end(), id) == syncedContainer.end()) { // check to avoid adding a change induced by remote sync
                localChanges->addChange(entry);
            }
        }
    }

    if (localChanges->getNumChanged() == 0){
        cout << "            / " << name << " VRSyncNode::update()" << " < < <" << endl;
        return;
    }

    // check for addChild changes
    createdNodes.clear();
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
                        cout << "register child for node " << node->getId() << " entry type " << entry->uiEntryDesc << endl;
                        registerNode(child);
                    }
                }
                cout << "node get core field id " << node->CoreFieldId << " core field mask " << node->CoreFieldMask << endl;
                NodeCore* core = node->getCore();
                for (auto p : core->getParents()) {
                    cout << "parent type " << p->getTypeName() << " id " << p->getId() << " of core " << core->getId() << endl;
                }
            }
            if (type == "Node" && entry->whichField & Node::CoreFieldMask) { // core change of known node
                cout << "  node core changed!" << endl;
                Node* node = dynamic_cast<Node*>(fct);
                registerContainer(node->getCore(), container.size());
            }
        }
    }

    int counter = 0;
    // check for created nodes and add entries to local changelist
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        //TODO: ignore core created somehow to have less created to handle on remote side
        //FieldContainer* fc = factory->getContainer(id);
        //if (fc->getTypeName() == "Node")
        entry->uiEntryDesc = ContainerChangeEntry::Create;
        if (container.count(id)) {
            localChanges->addChange(entry);
            cout << "add created " << entry->uiContainerId << endl;
            counter++;
        }
    }

    //get the change entries of the created children into local changelist
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (::find(createdNodes.begin(), createdNodes.end(), id) != createdNodes.end()) {
            //cout << "created FC entry type " << entry->uiEntryDesc << endl;
            localChanges->addChange(entry);
            //cout << "add created " << entry->uiContainerId << endl;
        }
        if (container.count(id)) counter ++;
    }
    cout << counter << " change entries in CL with registered IDs" << endl;

    cout << "print local changes: " << endl;
    printChangeList(localChanges);

    //DEBUG: print registered container
    cout << "print registered container: " << endl;
    for (auto c : container){
        UInt32 id = c.first;
        FieldContainer* fc = factory->getContainer(id);
        cout << id << " syncNodeID " << c.second ;
        if (factory->getContainer(id)){
            cout << " " << fc->getTypeName();
        }
        cout << endl;
    }

    // serialize changes in new change list (check OSGConnection for serialization Implementation)
    string data = serialize(localChanges);
    delete localChanges;

    // send over websocket to remote
    for (auto& remote : remotes) {
        remote.second->send(data);
        //cout << name << " sending " << data  << endl;
    }

    syncedContainer.clear();

    cout << "            / " << name << " VRSyncNode::update()" << "  < < < " << endl;
}

void VRSyncNode::registerContainer(FieldContainer* c, int syncNodeID) {
    cout << " VRSyncNode::registerContainer " << getName() << " container: " << c->getTypeName() << " at fieldContainerId: " << c->getId() << endl;
    container[c->getId()] = syncNodeID;
    createdNodes.push_back(c->getId());
}

//returns registered IDs
void VRSyncNode::registerNode(Node* node) {
    NodeCoreMTRefPtr core = node->getCore();
    cout << "register node " << node->getId() << endl;
    registerContainer(node, container.size());
    if (!core) cout << "no core" << core << endl;
    cout << "register core " << core->getId() << endl;
    registerContainer(core, container.size());
    for (int i=0; i<node->getNChildren(); i++) {
        cout << "register child " << node->getChild(i)->getId() << endl;
        registerNode(node->getChild(i));
    }
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
