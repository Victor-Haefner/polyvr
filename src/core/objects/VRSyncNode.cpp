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

using namespace OSG;

template<> string typeName(const VRSyncNode& o) { return "SyncNode"; }

ThreadRefPtr applicationThread;

void VRSyncNode::printChangeList(ChangeList* cl) {
    //if (cl->size() == 0) return;

    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    stringstream changedContainers;
    int j = 0;
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        if (j == 0) cout << "VRSyncNode::printChangeList " << name << endl;
        ContainerChangeEntry* entry = *it;
        const FieldFlags* fieldFlags = entry->pFieldFlags;
        BitVector whichField = entry->whichField;
        UInt32 id = entry->uiContainerId;
        //cout << "entry->uiContainerId " << entry->uiContainerId << endl;
        //continue;

        // ----- print info ---- //
        string type = "";
        if (factory->getContainer(id) != nullptr){
            type += factory->getContainer(id)->getTypeName();
        }

        cout << "whichField " << whichField << ", uiContainerId: " << id << " containerType: " << type << endl;

        /*for (int i=0; i<64; i++) {
            //int bit = (whichField & ( 1 << i )) >> i;
            BitVector one = 1;
            BitVector mask = ( one << i );
            bool bit = (whichField & mask);
            if (bit) {
                cout << "  whichField: " << i << " : " << bit << "  mask: " << mask << endl;
            }
        }*/
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
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) {
            //;//TODO
            data.insert(data.end(), (BYTE*)&src, (BYTE*)&src + sizeof(size)); //v.insert(v.end(), data.begin(), data.end())
            //data.insert(data.end(), handler.data.begin(), handler.data.end());
            cout << "writing data to handler " << sizeof(data) << endl;

        }

        void write(MemoryHandle src, UInt32 size) {
            data.insert(data.end(), src, src + size);
        }

        //void readBuffer() throw (ReadError) {}
        //void writeBuffer() {}

        vector<BYTE> data;
};

void serialize_entry(ContainerChangeEntry* entry, vector<BYTE>& data) {
    UInt32 id = entry->uiContainerId;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainer* fcPtr = factory->getContainer(id);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = id;
        sentry.fieldMask = entry->whichField;

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        cout << "serialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " | encoded: " << data.size() << endl;

        data.insert(data.end(), (BYTE*)&sentry, (BYTE*)&sentry + sizeof(SerialEntry)); //v.insert(v.end(), data.begin(), data.end())
        data.insert(data.end(), handler.data.begin(), handler.data.end());
    }
}

string serialize(ChangeList* clist) {
    vector<BYTE> data;
    for (auto it = clist->begin(); it != clist->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        if (entry->uiEntryDesc != ContainerChangeEntry::Change) continue;
        serialize_entry(entry, data);
    }
    //string res = base64_encode((BYTE*)clist, sizeof(*clist));
    //cout << " serialize result: " << data << endl;
    return base64_encode(&data[0], data.size());
}

void deserializeAndApply(string& data) {
    vector<BYTE> vec = base64_decode(data);
    int pos = 0;
    while (pos < vec.size()) {
        cout << " !!! search for sentry at " << pos << "/" << vec.size() << endl;
        SerialEntry sentry = *((SerialEntry*)&vec[pos]);
        cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << endl;
        pos += sizeof(SerialEntry);
        vector<BYTE> FCdata;
        FCdata.insert(FCdata.end(), vec.begin()+pos, vec.begin()+pos+sentry.len);
        pos += sentry.len;
        // TODO:
        //  - get corresponding ID to sentry.localId -> 3021
        UInt32 id = 3021;
        //  - get fieldcontainer using correct ID
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        FieldContainer* fcPtr = factory->getContainer(id);
        //  - use ourBinaryDataHandler to somehow apply binary change to fieldcontainer (override read method)
        // ourBinaryDataHandler handler;
        // TODO: feed handler with FCdata!
        // fcPtr->copyFromBin(handler, sentry.fieldMask);
        ourBinaryDataHandler handler;
        fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read

        //apply changes on the container
    }
}

//update this SyncNode
void VRSyncNode::update() {

    // go through all changes, gather changes where the container is known (in containers)
    ChangeList* cl = applicationThread->getChangeList();
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return;
    //cl->commitChanges();

    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    //printChangeList(cl);
    int j = 0;

    // create local changelist with changes of containers of the subtree of this sync node :D
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        if (entry->uiEntryDesc != ContainerChangeEntry::Change) continue; // TODO: only for debugging!
        UInt32 id = entry->uiContainerId;
        if (container.count(id)) {
            //cout << " change ? " << id << "  " << entry->whichField << "  " << Node::ChildrenFieldMask << endl;
            //cout << " change ? " << id << "  " << *entry->bvUncommittedChanges << "  " << Node::ChildrenFieldMask << endl;
            localChanges->addChange(entry);
        }
    }

    if (localChanges->getNumChanged() == 0) return;

    cout << "\nVRSyncNode::update " << name << endl;

    // check for addChild changes
    for (auto it = localChanges->begin(); it != localChanges->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        //cout << " change of container: " << id << ", fields: " << entry->whichField << endl;
        FieldContainer* fct = factory->getContainer(id);
        if (fct){
            string type = fct->getTypeName();

            if (type == "Node" && entry->whichField & Node::ChildrenFieldMask) {
                //cout << "  node children changed!" << endl;
                Node* node = dynamic_cast<Node*>(fct);
                for (int i=0; i<node->getNChildren(); i++) {
                    Node* child = node->getChild(i);
                    if (!container.count(child->getId())) {
                        //cout << "   found unregistred child: " << child->getId() << endl;
                        registerNode(child); // TODO: add created for every new registered container?
                    }
                }
            }

            if (type == "Node" && entry->whichField & Node::CoreFieldMask) {
                //cout << "  node core changed!" << endl;
                Node* node = dynamic_cast<Node*>(fct);
                registerContainer(node->getCore()); // TODO: add created?
            }
        }
    }

    // check for created nodes
    /*for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (container.count(id)) localChanges->addCreate(entry);
    }*/

    cout << "local changes: " << endl;
    printChangeList(localChanges);

    // serialize changes in new change list (check OSGConnection for serialization Implementation)
    string data = serialize(localChanges);
    delete localChanges;

    // send over websocket to remote
    for (auto& remote : remotes) {
        remote.second->send(data);
        cout << name << " sending " << data  << endl;
    }
}

void VRSyncNode::registerContainer(FieldContainer* c) {
    container[c->getId()] = true;
}

void VRSyncNode::registerNode(Node* node) {
    NodeCoreMTRefPtr core = node->getCore();
    registerContainer(node);
    registerContainer(core);
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
    //cout << "maybe started Interface at port " << port << endl;
}

string asUri(string host, int port, string name) {
    return "ws://" + host + ":" + to_string(port) + "/" + name;
}

//Add remote Nodes to sync with
void VRSyncNode::addRemote(string host, int port, string name) {
    string uri = asUri(host, port, name);
    //cout << "added SyncRemote (host, port, name) " << "(" << host << ", " << port << ", " << name << ") -> " << uri << endl;
    remotes[uri] = VRSyncRemote::create(uri);
    //remotes.insert(map<string, VRSyncRemote>::value_type(uri, VRSyncRemote(uri)));
    //cout << " added SyncRemote 2" << endl;
    //remotes[uri].connect();
}

void VRSyncNode::handleChangeList(void* _args) {
    HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) cout << "AAAARGH" << endl;

    int client = args->ws_id;
    string msg = args->ws_data;

    cout << "GOT CHANGES!! " << endl;
    cout << name << ", received msg: "  << msg << endl;//<< " container: ";

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
    for( auto c : container){
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
