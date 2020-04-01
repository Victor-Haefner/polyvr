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
    int clen = 0;
    int syncNodeID = -1;
    int uiEntryDesc = -1;
    int fcTypeID = -1;
    int coreID = -1;
    int cplen = 0;
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) {
            cout << "ourBinaryDataHandler -> read() data.size" << data.size() << " size " << size << endl;
            memcpy(src, &data[0], min((size_t)size, data.size())); //read data from handler into src (sentry.fieldMask)
        }

        void write(MemoryHandle src, UInt32 size) {
            data.insert(data.end(), src, src + size);
        }

        vector<BYTE> data;
};


void printNodeFieldMask(BitVector fieldmask) {
    string changeType = "";
    if (fieldmask & Node::AttachmentsFieldMask) changeType = " AttachmentsFieldMask";
//    else if (fieldmask & Node::bInvLocalFieldMask) changeType = " bInvLocalFieldMask ";
//    else if ( fieldmask & Node::bLocalFieldMask) changeType = " bLocalFieldMask      ";
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

void VRSyncNode::serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID) {
    UInt32 id = entry->uiContainerId;
    //FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainer* fcPtr = factory->getContainer(id);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = id;
        sentry.fieldMask = entry->whichField;
        sentry.syncNodeID = syncNodeID;
        sentry.uiEntryDesc = entry->uiEntryDesc;
        sentry.fcTypeID = fcPtr->getTypeId();

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        vector<int> childIDs;
        vector<pair<int,int>> children;

        // children and cores
        if (factory->findType(sentry.fcTypeID)->isNode()) {
//            cout << "NODE entry" << endl;
            Node* node = dynamic_cast<Node*>(fcPtr);

            if (sentry.fieldMask & Node::CoreFieldMask) { // node core changed
                sentry.coreID = node->getCore()->getId();
            }

            if (sentry.fieldMask & Node::ChildrenFieldMask) { // new child added
//                cout << "!!! change in childrenFM " << node->getNChildren() << endl;
                for (int i=0; i<node->getNChildren(); i++) {
                    Node* child = node->getChild(i);
                    childIDs.push_back(child->getId());
//                    int syncID = -1;
                    int syncID = container[child->getId()] ? child->getId() : -1;
                    children.push_back(make_pair(child->getId(), syncID));
//                    cout << "push_back " << child->getId() << endl;
                }
            }
        }
        sentry.clen = childIDs.size();
        sentry.cplen = children.size();
//        cout << "children " << sentry.clen << endl;
        for (int i = 0; i < sentry.clen; ++i) cout << childIDs[i] << endl;

        data.insert(data.end(), (BYTE*)&sentry, (BYTE*)&sentry + sizeof(SerialEntry));
//        cout << "data size sentry " << data.size() << endl;
        data.insert(data.end(), handler.data.begin(), handler.data.end());
//        cout << "data size sentry + handler " << data.size() << endl;
        if (sentry.clen > 0) data.insert(data.end(), (BYTE*)&childIDs[0], (BYTE*)&childIDs[0] + sizeof(int)*sentry.clen);
//        cout << " total data size " << data.size() << endl;
        if (sentry.cplen > 0) data.insert(data.end(), (BYTE*)&children[0], (BYTE*)&children[0] + sizeof(pair<int,int>)*sentry.cplen);
        cout << "serialize fc " << factory->findType(fcPtr->getTypeId())->getName() << " " << fcPtr->getTypeId() << " > > > sentry: " << sentry.localId << " syncID " << sentry.syncNodeID << " fieldMask " << sentry.fieldMask << " len " << sentry.len << " | encoded: " << data.size() << endl;
        //printNodeFieldMask(sentry.fieldMask);
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
        serialize_entry(entry, data, container[id]);
        counter++;
    }
    cout << "serialized entries: " << counter << endl;
    cout << "            / " << name << " / VRSyncNode::serialize()" <<"  < < <" << endl;
    return base64_encode(&data[0], data.size());
}

void VRSyncNode::deserializeAndApply(string& data) {
    cout << "> > >  " << name << " VRSyncNode::deserializeAndApply(), received data size: " << data.size() << endl;
    vector<BYTE> vec = base64_decode(data);
    int pos = 0;
    int counter = 0;
    ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer (use connection instead of handler, see OSGRemoteaspect.cpp (receiveSync))
    Node* parent = 0;
//    map<int, vector<BYTE>> entryData; //syncID, data
//    map<int, SerialEntry*> entryChange; // syncID, entry


    //deserialize and collect change and create entries
    while (pos + sizeof(SerialEntry) < vec.size()) {
        SerialEntry sentry = *((SerialEntry*)&vec[pos]);
        cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << " syncID " << sentry.syncNodeID << " at pos " << pos << endl;

        pos += sizeof(SerialEntry);
        vector<BYTE> FCdata;
        FCdata.insert(FCdata.end(), vec.begin()+pos, vec.begin()+pos+sentry.len);
        pos += sentry.len;

        // if the Children FM changed, update the children
        vector<int> childIDs;
        if (sentry.fieldMask & Node::ChildrenFieldMask) {
            vector<BYTE> children;
            children.insert(children.end(), vec.begin()+pos, vec.begin()+pos+sentry.clen*sizeof(int));
            cout << "children " << children.size() << endl;
            for (int i = 0; i < children.size(); i+=sizeof(int)) { //TODO: test with i+=sizeof(int)//NOTE: int can be either 4 (assumed here) or 2 bytes, depending on system
                cout << i << endl;
                int val;
                memcpy(&val, &children[i], sizeof(int));
                cout << val << endl;
                childIDs.push_back(val);
            }
        }
        for (int i = 0; i<childIDs.size(); i++) {
            cout << childIDs[i] << endl;
        }
        pos += sentry.clen*sizeof(int);
        map<int,int> children;
        if (sentry.fieldMask & Node::ChildrenFieldMask) {
            vector<BYTE> childs;
            cout << "insert attempt " << endl;
            childs.insert(childs.end(), vec.begin()+pos, vec.begin()+pos+sentry.cplen*sizeof(pair<int,int>));
            cout << "children " << children.size() << endl;
            for (int i = 0; i < childs.size(); i+=sizeof(pair<int,int>)) { //NOTE: int can be either 4 (assumed here) or 2 bytes, depending on system
                int key;
                int val;
                memcpy(&key, &childs[i], sizeof(int));
                memcpy(&val, &childs[i] + sizeof(int), sizeof(int));
                children[key] = val;
//                childIDs[i] = val;
            }
            pos += childs.size();
        }

        counter++;
        //if (sentry.uiEntryDesc == ContainerChangeEntry::Create) entryCreate[sentry.syncNodeID] = &sentry;
        //else
//        entryChange[sentry.syncNodeID] = &sentry;
//        entryData[sentry.syncNodeID] = FCdata;

        // map remote id to local id if exist (otherwise id = -1)
        UInt32 id = -1;
        if (sentry.syncNodeID != -1) {
            for (auto c : container) {
                if (c.second == sentry.syncNodeID) {
                    id = c.first;
                    remoteToLocalID[sentry.syncNodeID] = id;
//                    cout << "replaced remoteID " << sentry.localId;
                    sentry.localId = id; //replace remoteID by localID
//                    cout << " with localID " << sentry.localId << endl;
                    break;
                }
            }
        }

        FieldContainerRecPtr fcPtr = nullptr; // Field Container to apply changes to
//        if (id == -1) continue;
        // if fc does not exist we need to create a new fc = node
        if ( id == -1) { //if create and not registered | sentry.uiEntryDesc == ContainerChangeEntry::Create &&
//            FieldContainerRecPtr fcPtr;
            UInt32 typeID = sentry.fcTypeID;
            FieldContainerType* fcType = factory->findType(typeID);
//            cout << "create node of type " << fcType->getName() << endl;
            fcPtr = fcType->createContainer();
//            cout << "!!!!! created FC " << fcPtr->getTypeName() << " " << fcPtr->getId() << "  " << fcPtr->getRefCount() << endl;
            registerContainer(fcPtr.get(), sentry.syncNodeID);
            id = fcPtr.get()->getId();

            //find parent


            if (fcType->isNode()){
//                cout << "is node" << endl;
                Node* node = dynamic_cast<Node*>(fcPtr.get());
//                cout << "casted to node " << node->getTypeName() << " " << node->getId() << endl;
//                cout << "created Node " << node->getId() << endl;
//                cout << "parent " << parent << endl;
//                cout << "added to parent " << parent->getId() << endl;
                //TODO
                //FieldContainer* parent = getParentFC(node->getId());
                parent->addChild(node);
            }

            if (fcType->isNodeCore()){
                NodeCore* core = dynamic_cast<NodeCore*>(fcPtr.get());
                //get node via syncID: node syncID is core syncID-1
                UInt32 nodeSyncID = --sentry.syncNodeID;
                for (auto entry : container) {
                    UInt32 id = entry.first;
                    int syncID = entry.second;
                    if (syncID == nodeSyncID) {
                        FieldContainer* nodeFC = factory->getContainer(id);
                        Node* node = dynamic_cast<Node*>(nodeFC);
                        node->setCore(core);
//                        cout << "added core " << core->getId() << " to node " << node->getId() << endl;
                    }
                    //TODO: if no node was found, this core is not referenced and the fc get deleted
                    //create node with syncID-1
                    else {
                        NodeRefPtr nodePtr = Node::create();
                        Node* node = nodePtr.get();
                        //TODO
                        //FieldContainer* parent = getParentFC(node->getId());
                        node->setCore(core);
                        parent->addChild(node); //TODO reassure
                        //register if not in container
                        bool registered = false;
                        for (auto entry : container) { //TODO: check if its not a cyclic dependence
                            int entrySyncID = entry.second;
                            if (nodeSyncID == entrySyncID) {
                                registered = true;
                            }
                        }
                        if (!registered) registerContainer(node, nodeSyncID);
                    }
                }
            }
        }
        else {
            fcPtr = factory->getContainer(id);
        }
        cout << "field container " << fcPtr.get() << endl;

        //fc = factory->getContainer(id);
        //  - get fieldcontainer using correct ID
        if(fcPtr == nullptr) {cout << "no container found with id " << id << " syncNodeID " << sentry.syncNodeID << endl; continue;} //TODO: This is causing the WARNING: Action::recurse: core is NULL, aborting traversal.

        //did the children field mask change?
        if (sentry.fieldMask & Node::ChildrenFieldMask) {
            //set parent for next entry
            FieldContainer* parentFC = fcPtr.get();
            int fcTypeID = sentry.fcTypeID;
            if (factory->findType(fcTypeID)->isNode()) {
                parent = dynamic_cast<Node*>(parentFC);
//                cout << "----- set parent " << parent->getTypeName() << " " << parent->getId() << endl;
            }
        }

//        cout << name << " apply data to " << fcPtr->getTypeName() << " (" << fcPtr->getTypeId() << ")" << endl;

        handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata
        fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read
        if (id != -1) {
            if (count(syncedContainer.begin(), syncedContainer.end(), id) < 1) {
                syncedContainer.push_back(id);
//                cout << "syncedContainer.push_back " << id << endl;
            }
        }
    }

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

    //Thread::getCurrentChangeList()->commitChanges(ChangedOrigin::Sync);
    //Thread::getCurrentChangeList()->commitChangesAndClear(ChangedOrigin::Sync);

    cout << "deserialized entries: " << counter << endl;
    cout << "            / " << name << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;
}

//create a node and core for created child change entry
NodeRefPtr VRSyncNode::createChild(Node* parent){
    NodeRefPtr nodePtr = Node::create();
    GroupRefPtr group = Group::create();
    nodePtr->setCore(group);
    Node* node = nodePtr.get();
    parent->addChild(node); //Add to scene graph TODO: better to
//    cout << "created node " << node->getId() << endl;
//    //FieldContainer* fc = factory->getContainer(node->getId());
//    if (!fc) cout << "no fc found for node" << endl;
//    else cout << "found fc " << fc->getTypeName() << " " << fc->getId() << endl;
    cout << "created child " << node->getId() << " with core " << group->getId() << endl;
    return nodePtr;
}

//copies state into a CL and serializes it as string
string VRSyncNode::copySceneState() {
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    localChanges->fillFromCurrentState();
    printChangeList(localChanges);

    string data = serialize(localChanges);
    delete localChanges;
    return data;
}

//update this SyncNode
void VRSyncNode::update() {
    // go through all changes, gather changes where the container is known (in containers)
    ChangeList* cl = applicationThread->getChangeList();
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return;
    cout << "> > >  " << name << " VRSyncNode::update()" << endl;
//    if (name == "node1") printChangeList(cl);
    //DEBUG: print registered container
    bool first = true;
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



    cout << "syncedContainer " << endl;
    for (int id : syncedContainer) cout << id << endl;

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
        if (fct) {
            //string type = fct->getTypeName();
            if (factory->findType(fct->getTypeId())->isNode() && entry->whichField & Node::ChildrenFieldMask) { //if the children filed mask of node has changed, we check if a new child was added
                Node* node = dynamic_cast<Node*>(fct);
                for (int i=0; i<node->getNChildren(); i++) {
                    Node* child = node->getChild(i);
                    if (!container.count(child->getId())) { //check if it is an unregistered child
                        cout << "register child for node " << node->getId() << " entry type " << entry->uiEntryDesc << endl;
                        vector<int> newNodes = registerNode(child);
                        //createdNodes.push_back(child->getId()); //TODO
//                        createdNodes.insert(createdNodes.end(), newNodes.begin(), newNodes.end());
                        for (int i = 0; i<newNodes.size(); ++i){
                            createdNodes.push_back(newNodes[i]);
//                            cout << "pushed_back newNodes[i] " << newNodes[i] << endl;
//                            cout << "search id in CL " << endl;
//                            for (auto it = cl->begin(); it != cl->end(); ++it) {
//                                ContainerChangeEntry* entry = *it;
//                                UInt32 id = entry->uiContainerId;
//                                if (id == newNodes[i]) cout << "found entry in global CL!!!!" << newNodes[i] << endl;
//                            }
//                            cout << "search id in syncedContainer " << endl;
//                            for (int id : syncedContainer) {
//                                if (id == newNodes[i]) cout << "found entry in syncedContainer!!!!" << newNodes[i] << endl;
//                            }
                        }
                    }
                }
                cout << "node get core field id " << node->CoreFieldId << " core field mask " << node->CoreFieldMask << endl;
                NodeCore* core = node->getCore();
                for (auto p : core->getParents()) {
                    cout << "parent type " << p->getTypeName() << " id " << p->getId() << " of core " << core->getId() << endl;
                }
            }
            if (factory->findType(fct->getTypeId())->isNode() && entry->whichField & Node::CoreFieldMask) { // core change of known node
                cout << "  node core changed!" << endl;
                Node* node = dynamic_cast<Node*>(fct);
                registerContainer(node->getCore(), container.size());
                createdNodes.push_back(node->getCore()->getId()); //TODO
            }
        }
    }

    int counter = 0;
    // add created entries to local CL
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
//        cout << "created " << id << endl;
        //TODO: ignore core created somehow to have less created to handle on remote side
        if (container.count(id)) {
            if (::find(syncedContainer.begin(), syncedContainer.end(), id) == syncedContainer.end()) { // TODO: optimize by reorganizing if clauses
                localChanges->addChange(entry);
            }
            //cout << "add created " << entry->uiContainerId << endl;
            createdNodes.push_back(id);
            counter++;
        }
    }

//    cout << "!!! print createdNodes" << endl;
//    for (int i : createdNodes) cout << i << endl;

    //TODO: more performant if reordered loops? bc usually tehre are less createdNodes than change entries in CL
    // add change entries (of created FC which were not tracked) to local change list
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
//        cout << "searching createdNodes for " << id << endl;
        if (::find(createdNodes.begin(), createdNodes.end(), id) != createdNodes.end()) {
            if (::find(syncedContainer.begin(), syncedContainer.end(), id) == syncedContainer.end()) { // TODO: optimize by reorganizing if clauses
                localChanges->addChange(entry);
            }
//            cout << "add created " << entry->uiEntryDesc << " " << entry->uiContainerId << endl;
        }
        if (container.count(id)) counter ++;
    }
    cout << counter << " change entries in CL with registered IDs" << endl;

//    cout << "print local changes: " << endl;
//    printChangeList(localChanges);

    //DEBUG: print registered container
    cout << "print registered container: " << endl;
    for (auto c : container){
        UInt32 id = c.first;
        FieldContainer* fc = factory->getContainer(id);
        cout << id << " syncNodeID " << c.second ;
        if (factory->getContainer(id)){
            cout << " " << fc->getTypeName() << " Refs " << fc->getRefCount();
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
}

//returns registered IDs
vector<int> VRSyncNode::registerNode(Node* node) {
    vector<int> res;
    vector<int> localRes;
    vector<int> recursiveRes;
    NodeCoreMTRefPtr core = node->getCore();
    cout << "register node " << node->getId() << endl;

    registerContainer(node, container.size());
    if (!core) cout << "no core" << core << endl;
    cout << "register core " << core->getId() << endl;

    registerContainer(core, container.size());
    localRes.push_back(node->getId());
    localRes.push_back(core->getId());
    for (int i=0; i<node->getNChildren(); i++) {
        cout << "register child " << node->getChild(i)->getId() << endl;
        recursiveRes = registerNode(node->getChild(i));
    }
    res.reserve(localRes.size() + recursiveRes.size());
    res.insert(res.end(), localRes.begin(), localRes.end());
    res.insert(res.end(), recursiveRes.begin(), recursiveRes.end());
    return res;
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

void VRSyncNode::handleChangeList(void* _args) { //TODO: rename in handleMessage
    HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) cout << "AAAARGH" << endl;

    int client = args->ws_id;
    string msg = args->ws_data;

    //cout << "GOT CHANGES!! " << endl;
    //cout << name << ", received msg: "  << msg << endl;//<< " container: ";
    if (msg == "hello") {
        cout << "received hello from remote" << endl;
        //TODO: send it only back to the sender
//        string data = copySceneState();
//        for (auto& remote : remotes) {
//            remote.second->send(data);
//            //cout << name << " sending " << data  << endl;
//        }
    }
    else deserializeAndApply(msg);
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
    if (result) send("hello");
}

bool VRSyncRemote::send(string message){
    if (!socket->sendMessage(message)) return 0;
    return 1;
}
