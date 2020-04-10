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

#include <bitset>

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

string getChangeType(int uiEntryDesc) {
    string changeType;
    switch (uiEntryDesc) {
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
            changeType = " none              ";
    }
    return changeType;
}

void VRSyncNode::printChangeList(OSGChangeList* cl) {
    if (!cl) return;
    cout << endl << "ChangeList:";
    if (cl->getNumChanged() == 0 && cl->getNumCreated() == 0) cout << " no changes " << endl;
    else cout << " node: " << name << ", changes: " << cl->getNumChanged() << ", created: " << cl->getNumCreated() << endl;

    auto printEntry = [&](ContainerChangeEntry* entry) {
        const FieldFlags* fieldFlags = entry->pFieldFlags;
        BitVector whichField = entry->whichField;
        UInt32 id = entry->uiContainerId;

        // ----- print info ---- //
        string type;
        if (factory->getContainer(id)) type = factory->getContainer(id)->getTypeName();
        string changeType = getChangeType(entry->uiEntryDesc);
        cout << "  " << "uiContainerId: " << id << ", changeType: " << changeType << ", container: " << type << endl;
    };

    cout << " Created:" << endl;
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) printEntry(*it);
    cout << " Changes:" << endl;
    for (auto it = cl->begin(); it != cl->end(); ++it) printEntry(*it);
}

VRSyncNode::VRSyncNode(string name) : VRTransform(name) {
    type = "SyncNode";
    applicationThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());

//    NodeMTRefPtr node = getNode()->node; // deprecated, gets filtered from created entries in CL
//    registerNode(node);

	updateFkt = VRUpdateCb::create("SyncNode update", bind(&VRSyncNode::update, this));
	//VRScene::getCurrent()->addUpdateFkt(updateFkt, 100000);
}

VRSyncNode::~VRSyncNode() {
    cout << "VRSyncNode::~VRSyncNode " << name << endl;
}

VRSyncNodePtr VRSyncNode::ptr() { return static_pointer_cast<VRSyncNode>( shared_from_this() ); }
VRSyncNodePtr VRSyncNode::create(string name) { return VRSyncNodePtr(new VRSyncNode(name) ); }

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

UInt32 VRSyncNode::getRegisteredContainerID(int syncID) {
    for (auto registered : container) {
        if (registered.second == syncID) return registered.first;
    }
    return -1;
}

struct SerialEntry {
    int localId = 0; //local FieldContainer Id
    BitVector fieldMask;
    int len = 0; //number of data BYTEs in the SerialEntry
    int cplen = 0; //number of Children BYTEs in the SerialEntry
    int syncNodeID = -1; //sync Id of the Node
    int uiEntryDesc = -1; //ChangeEntry Type
    int fcTypeID = -1; //FieldContainer Type Id
    int coreID = -1; //Core Id is this is an entry for node with a core
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) {
//            cout << "ourBinaryDataHandler -> read() data.size" << data.size() << " size " << size << endl;
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

void printContainer (FieldContainerFactoryBase* factory, map<int,int> container) {
    for (auto c : container) {
        UInt32 id = c.first;
        FieldContainer* fcPt = factory->getContainer(id);
        if (!fcPt) continue;
        FieldContainerType* fcType = factory->findType(fcPt->getTypeId());
        if (fcType) {
            if (fcType->isNode()) {
                Node* node = dynamic_cast<Node*>(fcPt);
                cout << id << " " << fcPt->getTypeName() << " children " << node->getNChildren() << endl;
                if (node->getNChildren() == 0) cout << "no children" << endl;
                for (int i = 0; i < node->getNChildren(); i++) {
                    Node* childNode = node->getChild(i);
                    cout << "   " << childNode->getId() << endl;
                }
            }
        }
    }
}

vector<int> VRSyncNode::getFCChildren(FieldContainer* fcPtr, BitVector fieldMask) {
    vector<int> res;
    Node* node = dynamic_cast<Node*>(fcPtr);
    if (fieldMask & Node::ChildrenFieldMask) { // new child added
        for (int i=0; i<node->getNChildren(); i++) {
            Node* child = node->getChild(i);
            res.push_back(child->getId());
            //int syncID = container[child->getId()] ? container[child->getId()] : -1;
            //res.push_back(make_pair(child->getId(), syncID));
            //cout << "VRSyncNode::getFCChildren  children.push_back " << child->getId() << endl; //Debugging
        }
    }
    return res;
}

void VRSyncNode::serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data, int syncNodeID) {
    FieldContainer* fcPtr = factory->getContainer(entry->uiContainerId);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = entry->uiContainerId;
        sentry.fieldMask = entry->whichField;
        sentry.syncNodeID = syncNodeID;
        sentry.uiEntryDesc = entry->uiEntryDesc;
        sentry.fcTypeID = fcPtr->getTypeId();

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        vector<int> children;
        if (factory->findType(sentry.fcTypeID)->isNode()) children = getFCChildren(fcPtr, sentry.fieldMask);
        sentry.cplen = children.size();

        if (factory->findType(sentry.fcTypeID)->isNode()) {
            Node* node = dynamic_cast<Node*>(fcPtr);
            sentry.coreID = node->getCore()->getId();
//            if (sentry.fieldMask & Node::CoreFieldMask) { // node core changed
//                sentry.coreID = node->getCore()->getId();
//            }
        }

        data.insert(data.end(), (BYTE*)&sentry, (BYTE*)&sentry + sizeof(SerialEntry));
        data.insert(data.end(), handler.data.begin(), handler.data.end());

        if (sentry.cplen > 0) data.insert(data.end(), (BYTE*)&children[0], (BYTE*)&children[0] + sizeof(int)*sentry.cplen);
    }
}

string VRSyncNode::serialize(ChangeList* clist) {
    int counter = 0; //Debugging
    cout << "> > >  " << name << " VRSyncNode::serialize()" << endl; //Debugging

    vector<BYTE> data;
    for (auto it = clist->begin(); it != clist->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        serialize_entry(entry, data, container[entry->uiContainerId]);
        counter++;//Debugging
    }

    cout << "serialized entries: " << counter << endl; //Debugging
    cout << "            / " << name << " / VRSyncNode::serialize()" <<"  < < <" << endl; //Debugging

    return base64_encode(&data[0], data.size());
}


void VRSyncNode::deserializeEntries(string& data, vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData) {
    vector<BYTE> vec = base64_decode(data);
    int pos = 0;

    //deserialize and collect change and create entries
    while (pos + sizeof(SerialEntry) < vec.size()) {
        SerialEntry sentry = *((SerialEntry*)&vec[pos]);
        entries.push_back(sentry);

        pos += sizeof(SerialEntry);
        vector<BYTE>& FCdata = fcData[sentry.localId];
        FCdata.insert(FCdata.end(), vec.begin()+pos, vec.begin()+pos+sentry.len);
        pos += sentry.len;

        vector<BYTE> childrenData;
        childrenData.insert(childrenData.end(), vec.begin()+pos, vec.begin()+pos+sentry.cplen*sizeof(int));
        if (childrenData.size() > 0) {
            int* castedData = (int*)&childrenData[0];
            for (int i=0; i<sentry.cplen; i++) {
                int childID = castedData[i];
                parentToChildren[sentry.localId].push_back(childID);
            }
        }

        pos += sentry.cplen * sizeof(int);
    }
}

FieldContainerRecPtr VRSyncNode::getOrCreate(UInt32& id, SerialEntry& sentry, map<int, vector<int>>& parentToChildren) {
    //cout << "VRSyncNode::getOrCreate remote: " << sentry.localId << ", local: " << id << endl;
    FieldContainerRecPtr fcPtr = 0; // Field Container to apply changes to
    if (id != 0) fcPtr = factory->getContainer(id);
    else if (sentry.uiEntryDesc == ContainerChangeEntry::Create) {
        FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
        fcPtr = fcType->createContainer();
        justCreated.push_back(fcPtr); // increase ref count to avoid destruction!
        registerContainer(fcPtr.get(), sentry.syncNodeID);
        id = fcPtr.get()->getId();
        remoteToLocalID[sentry.localId] = id;
        //cout << " ---- create, new ID, remote: " << sentry.localId << ", local: " << id << endl;
    }
    //cout << " VRSyncNode::getOrCreate done with " << fcPtr << endl;
    return fcPtr;
}

void VRSyncNode::handleChildrenChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<int, vector<int>>& parentToChildren) {
    //cout << "VRSyncNode::handleChildrenChange +++++++++++++++++++++++++++++++++++++++++++++++ " << fcPtr->getId() << endl;
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    vector<int> childrenIDs = parentToChildren[sentry.localId];
    //cout << " N children: " << childrenIDs.size() << endl;
    for (cID : childrenIDs) {
        UInt32 childID = remoteToLocalID.count(cID) ? remoteToLocalID[cID] : 0;
        FieldContainer* childPtr = factory->getContainer(childID);
        Node* child = dynamic_cast<Node*>(childPtr);
        //cout << "  child: " << childID << " " << child << endl;
        if (!child) continue;
        node->addChild(child);
        //cout << " add child, parent: " << node->getId() << ", child: " << child->getId() << " ------------------- " << endl;
    }
    //cout << "  VRSyncNode::handleChildrenChange done" << endl;
}

void VRSyncNode::handleCoreChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry) {
    //cout << "VRSyncNode::handleCoreChange" << endl;
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    UInt32 coreID = remoteToLocalID.count(sentry.coreID) ? remoteToLocalID[sentry.coreID] : -1;
    FieldContainer* corePtr = factory->getContainer(coreID);
    NodeCore* core = dynamic_cast<NodeCore*>(corePtr);
    if (!core) return;
    node->setCore(core);
    //cout << " VRSyncNode::handleCoreChange done" << endl;
}

void VRSyncNode::handleGenericChange(FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<int, vector<BYTE>>& fcData) {
    cout << "VRSyncNode::handleGenericChange " << fcPtr->getId() << " ---------------------------- " << endl;

    // deactivate certain changes
    if (factory->findType(sentry.fcTypeID)->isNode()) {
        sentry.fieldMask &= ~Node::AttachmentsFieldMask; // TODO: pass attachment containers when filtering the changelist

        sentry.fieldMask &= ~Node::ChildrenFieldMask; // clear children field mask bit!
        sentry.fieldMask &= ~Node::ParentFieldMask; // clear parent field mask bit!
        sentry.fieldMask &= ~Node::CoreFieldMask; // clear core field mask bit!
    }

    if (factory->findType(sentry.fcTypeID)->isNodeCore()) {
        sentry.fieldMask &= ~NodeCore::ParentsFieldMask; // clear parents field mask bit!
    }

    // apply changes
    vector<BYTE>& FCdata = fcData[sentry.localId];
    ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer
    handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata
    fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read
    cout << " FCdata size: " << FCdata.size() << "  " << std::bitset<64>(sentry.fieldMask) << endl;
}

void VRSyncNode::handleRemoteEntries(vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData) {
    for (auto sentry : entries) {
        //cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << " syncID " << sentry.syncNodeID << " at pos " << pos << endl;

        //sync of initial syncNode container
        if (sentry.syncNodeID >= 0 && sentry.syncNodeID <= 2) {
            for (auto c : container) {
                if (c.second == sentry.syncNodeID) remoteToLocalID[sentry.localId] = c.first;
            }
        }

        UInt32 id = remoteToLocalID.count(sentry.localId) ? remoteToLocalID[sentry.localId] : 0; // map remote id to local id if exist (otherwise id = -1)
        FieldContainerRecPtr fcPtr = getOrCreate(id, sentry, parentToChildren); // Field Container to apply changes to

        if (fcPtr == nullptr) { cout << "WARNING! no container found with id " << id << " syncNodeID " << sentry.syncNodeID << endl; continue; } //TODO: This is causing the WARNING: Action::recurse: core is NULL, aborting traversal.

        if (sentry.uiEntryDesc == ContainerChangeEntry::Change) { //if its a node change, update child info has changed. TODO: check only if children info has changed
            if (sentry.fieldMask & Node::ChildrenFieldMask) handleChildrenChange(fcPtr, sentry, parentToChildren);
            if (sentry.fieldMask & Node::CoreFieldMask) handleCoreChange(fcPtr, sentry);
            handleGenericChange(fcPtr, sentry, fcData);
        }

        if (::find(syncedContainer.begin(), syncedContainer.end(), id) != syncedContainer.end()) {
            syncedContainer.push_back(id);
            //cout << "syncedContainer.push_back " << id << endl;
        }
    }
    //justCreated.clear(); // TODO: uncomment if finished debugging
}

void VRSyncNode::printDeserializedData(vector<SerialEntry>& entries, map<int, vector<int>>& parentToChildren, map<int, vector<BYTE>>& fcData) {
    cout << " Deserialized entries:" << endl;
    for (auto entry : entries) {
        cout << "  entry: " << entry.localId;

        if (entry.fcTypeID != -1) {
            FieldContainerType* fcType = factory->findType(entry.fcTypeID);
            cout << ", fcType: " << fcType->getName();
        }

        if (entry.syncNodeID != -1) cout << ", syncNodeID: " << entry.syncNodeID;
        if (entry.uiEntryDesc != -1) cout << ", change type: " << getChangeType(entry.uiEntryDesc);
        if (entry.coreID != -1) cout << ", coreID: " << entry.coreID;


        if (parentToChildren[entry.localId].size() > 0) {
            cout << ", children: (";
            for (int c : parentToChildren[entry.localId]) cout << " " << c;
            cout << " )";
        }
        cout << endl;
    }
}

void VRSyncNode::deserializeAndApply(string& data) {
    cout << endl << "> > >  " << name << " VRSyncNode::deserializeAndApply(), received data size: " << data.size() << endl;

    map<int, vector<int>> parentToChildren; //maps parent ID to its children syncIDs
    vector<SerialEntry> entries;
    map<int, vector<BYTE>> fcData; // map entry localID to its binary field data

    deserializeEntries(data, entries, parentToChildren, fcData);
    printDeserializedData(entries, parentToChildren, fcData);
    handleRemoteEntries(entries, parentToChildren, fcData);
    printRegistredContainers();

    cout << "            / " << name << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;
}

int VRSyncNode::findParent(map<int,vector<int>>& parentToChildren, int remoteNodeID) {
    int parentId = container.begin()->first;
    for (auto remoteParent : parentToChildren) {
        int remoteParentId = remoteParent.first;
        vector<int> remoteChildren = remoteParent.second;
        for (int i = 0; i<remoteChildren.size(); i++) {
            int remoteChildId = remoteChildren[i];
            if (remoteChildId == remoteNodeID) {
                if (remoteToLocalID[remoteParentId]) return remoteToLocalID[remoteParentId];
            }
        }
    }
    return parentId;
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

void VRSyncNode::printRegistredContainers() {
    cout << endl << "registered container:" << endl;
    for (auto c : container){
        UInt32 id = c.first;
        FieldContainer* fc = factory->getContainer(id);
        cout << " " << id << " syncNodeID " << c.second;
        if (fc) {
            cout << ", type: " << fc->getTypeName() << ", Refs: " << fc->getRefCount();
            if (Node* node = dynamic_cast<Node*>(fc)) cout << ", N children: " << node->getNChildren();
        }
        cout << endl;
    }
}

void VRSyncNode::printSyncedContainers() {
    cout << endl << "synced container:" << endl;
    for (int id : syncedContainer) cout << id << endl;
}

// checks if a container was changed by remote
bool VRSyncNode::isRemoteChange(const UInt32& id) {
    return bool(::find(syncedContainer.begin(), syncedContainer.end(), id) != syncedContainer.end());
}

bool VRSyncNode::isRegistered(const UInt32& id) {
    return bool(container.count(id));
}

//checks in container if the node with syncID is already been registered
bool VRSyncNode::isRegisteredRemote(const UInt32& syncID) {
    for (auto reg : container) { //check if the FC is already registered, f.e. if nodeCore create entry arrives first a core along with its node will be created before the node create entry arrives
        if (reg.second == syncID) return true;
    }
    return false;
}

bool VRSyncNode::isSubContainer(const UInt32& id) {
    auto fct = factory->getContainer(id);
    if (!fct) return false;

    UInt32 syncNodeID = getNode()->node->getId();
    auto type = factory->findType(fct->getTypeId());

    function<bool(Node*)> checkAncestor = [&](Node* node) {
        if (!node) return false;
        if (node->getId() == syncNodeID) return true;
        Node* parent = node->getParent();
        return checkAncestor(parent);
    };

    if (type->isNode()) {
        Node* node = dynamic_cast<Node*>(fct);
        if (!node) return false;
        return checkAncestor(node);
    }

    if (type->isNodeCore()) {
        NodeCore* core = dynamic_cast<NodeCore*>(fct);
        if (!core) return false;
        for (auto node : core->getParents())
            if (checkAncestor(dynamic_cast<Node*>(node))) return true;
    }

    Attachment* att = dynamic_cast<Attachment*>(fct);
    if (att) {
        auto parents = att->getMFParents();
        for (int i = 0; i<parents->size(); i++) {
            FieldContainer* parent = parents->at(i);
            if (isSubContainer(parent->getId())) return true;
        }
    }

    return false;
}

OSGChangeList* VRSyncNode::getFilteredChangeList() {
    // go through all changes, gather changes where the container is known (in containers)
    // create local changelist with changes of containers of the subtree of this sync node :D

    ChangeList* cl = applicationThread->getChangeList();
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return 0;

    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();

    // register created and add them to local CL
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;

        /*FieldContainer* fct = factory->getContainer(id);
        Attachment* att = dynamic_cast<Attachment*>(fct);
        if (fct) cout << " getFilteredChangeList entry: " << fct->getTypeName() << " attachment? " << att << endl;
        if (att) cout << "    attachement N parents: " << att->getMFParents()->size() << endl;*/

        if (isRemoteChange(id)) continue;

        if (isSubContainer(id)) {
            localChanges->addChange(entry);
            registerContainer(factory->getContainer(id), container.size());
        }
    }

    // add changed entries to local CL
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (isRemoteChange(id)) continue;
        if (isRegistered(id)) {
            localChanges->addChange(entry);
        }
    }

    return localChanges;
}

void VRSyncNode::broadcastChangeList(OSGChangeList* cl, bool doDelete) {
    if (!cl) return;
    string data = serialize(cl); // serialize changes in new change list (check OSGConnection for serialization Implementation)
    if (doDelete) delete cl;
    for (auto& remote : remotes) remote.second->send(data); // send over websocket to remote
}

void VRSyncNode::sync(string uri) {
    if (!container.size()) return;
    vector<BYTE> data;
    vector<int> containerData;
    for (auto c : container) {
        containerData.push_back(c.first);
        containerData.push_back(c.second);
    }
    data.insert(data.end(), (BYTE*)&containerData[0], (BYTE*)&containerData[0] + sizeof(int)*containerData.size());
    string msg = base64_encode(&data[0], data.size());
    remotes[uri]->send("sync");
    remotes[uri]->send(msg);
}

//update this SyncNode
void VRSyncNode::update() {
    cout << endl << " > > >  " << name << " VRSyncNode::update()" << endl;
    auto localChanges = getFilteredChangeList();
    if (!localChanges) return;
    cout <<  "  local changelist, created: " << localChanges->getNumCreated() << ", changes: " << localChanges->getNumChanged() << endl;

    printRegistredContainers(); // DEBUG: print registered container
    //printSyncedContainers();
    printChangeList(localChanges);

    broadcastChangeList(localChanges, true);
    syncedContainer.clear();
    cout << "            / " << name << " VRSyncNode::update()" << "  < < < " << endl;
}

void VRSyncNode::registerContainer(FieldContainer* c, int syncNodeID) {
    UInt32 ID = c->getId();
    cout << " VRSyncNode::registerContainer " << getName() << " container: " << c->getTypeName() << " at fieldContainerId: " << ID << endl;
    container[ID] = syncNodeID;
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
//    sync(uri);
}

void VRSyncNode::handleChangeList(void* _args) { //TODO: rename in handleMessage
    HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) cout << "AAAARGH" << endl;

    int client = args->ws_id;
    string msg = args->ws_data;
//
//    if (syncronizing) {
//        vector<BYTE> vec = base64_decode(msg);
//        map<int,int> remoteContainer;
//        vector<int> remoteContainerData;
//        vector<BYTE> remoteData;
//        int pos = 0;
//        while (pos+sizeof(int) < vec.size()){
//            remoteContainerData.insert(remoteContainerData.end(), vec.begin()+pos, vec.begin()+pos+sizeof(int));
//        }
//        //get remote container data into an int vec with IDs and syncIDs
//        if (remoteContainerData.size() > 0) {
//            int* castedData = (int*)&remoteContainerData[0];
//            for (int i=0; i<remoteContainerData.size(); i++) {
//                int id = castedData[i];
//                remoteContainerData.push_back(id);
//            }
//        }
//        //store remote container data in remoteContainer map
//        for (int i=0; i<remoteContainerData.size(); i+=2){
//            int id = remoteContainerData[i];
//            int syncId = remoteContainerData[i++];
//            remoteContainer[id] = syncId;
//        }
//        //update remoteToLocalIDs
//        for (auto c : container) {
//            int localSyncId = c.second;
//            bool found = false;
//            for (map<int,int>::iterator it = remoteContainer.begin(); it != remoteContainer.end() && !found; it++) {
//                int remoteSyncId = it->second;
//                if (localSyncId == remoteSyncId) {
//                    int localId = c.first;
//                    int remoteId = it->first;
//                    remoteToLocalID[remoteId] = localId;
//                    found = true;
//                }
//            }
//        }
//        //sync this nodes id's with others
//        for (auto& remote : remotes) {
//            sync(remote.first);
//        }
//        //end syncronizing
//        syncronizing = false;
//        cout << "end sync " << name << endl;
//    }
//    if (msg == "sync") { syncronizing = true; cout << "begin sync " << name << endl;}
//    if (!syncronizing) deserializeAndApply(msg);
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
    if (uri != "") connect();
}

VRSyncRemote::~VRSyncRemote() { cout << "~VRSyncRemote::VRSyncRemote" << endl; }
VRSyncRemotePtr VRSyncRemote::create(string name) { return VRSyncRemotePtr( new VRSyncRemote(name) ); }

void VRSyncRemote::connect() {
    bool result = socket->open(uri);
    if (!result) cout << "VRSyncRemote, Failed to open websocket to " << uri << endl;
}

bool VRSyncRemote::send(string message){
    if (!socket->sendMessage(message)) return 0;
    return 1;
}








