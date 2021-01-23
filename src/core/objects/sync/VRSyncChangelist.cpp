#include "VRSyncChangelist.h"
#include "VRSyncNode.h"
#include "core/objects/OSGObject.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGFieldContainer.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGAttachment.h>
#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGGroup.h>

// needed to filter GLId field masks
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSurface.h>
#include <OpenSG/OSGGeoProperty.h>
#include <OpenSG/OSGGeoMultiPropertyData.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureObjRefChunk.h>
#include <OpenSG/OSGUniformBufferObjStd140Chunk.h>
#include <OpenSG/OSGShaderStorageBufferObjStdLayoutChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGUniformBufferObjChunk.h>
#include <OpenSG/OSGShaderStorageBufferObjChunk.h>
#include <OpenSG/OSGShaderExecutableChunk.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGShaderVariableOSG.h>
#include <OpenSG/OSGProgramChunk.h>

#include <bitset>

using namespace OSG;

VRSyncChangelist::VRSyncChangelist() {}
VRSyncChangelist::~VRSyncChangelist() { cout << "~VRSyncChangelist::VRSyncChangelist" << endl; }
VRSyncChangelistPtr VRSyncChangelist::create() { return VRSyncChangelistPtr( new VRSyncChangelist() ); }

string toString(const BitVector& v) {
    std::stringstream ss;
    ss << std::bitset<sizeof(BitVector)*CHAR_BIT>(v);
    return ss.str();
}

struct SerialEntry {
    UInt32 localId = 0; //local FieldContainer Id
    BitVector fieldMask;
    UInt32 len = 0; //number of data BYTEs in the SerialEntry
    UInt32 cplen = 0; //number of Children BYTEs in the SerialEntry
    UInt32 syncNodeID = -1; //sync Id of the Node
    UInt32 uiEntryDesc = -1; //ChangeEntry Type
    UInt32 fcTypeID = -1; //FieldContainer Type Id
    UInt32 coreID = -1; //Core Id is this is an entry for node with a core
};

class OSGChangeList : public ChangeList {
    public:
        ~OSGChangeList() {};

        ContainerChangeEntry* newChange(UInt32 ID, BitVector fields, map<UInt32, ContainerChangeEntry*>& changedFCs) {
            ContainerChangeEntry* entry = 0;
            if (changedFCs.count(ID)) entry = changedFCs[ID];
            else {
                entry = getNewEntry();
                entry->uiEntryDesc = ContainerChangeEntry::Change;
                entry->uiContainerId = ID;
                changedFCs[ID] = entry;
            }
            entry->whichField = fields;
            return entry;
        }

        void addChange(ContainerChangeEntry* entry, map<UInt32, ContainerChangeEntry*>& changedFCs, UInt32 mask = -1) {
            /*if (entry->uiEntryDesc == ContainerChangeEntry::AddReference   ||
                entry->uiEntryDesc == ContainerChangeEntry::SubReference   ||
                entry->uiEntryDesc == ContainerChangeEntry::DepSubReference) {
                ContainerChangeEntry* pEntry = getNewEntry();
                pEntry->uiEntryDesc   = entry->uiEntryDesc;
                pEntry->uiContainerId = entry->uiContainerId;
                pEntry->pList         = this;
            } else if(entry->uiEntryDesc == ContainerChangeEntry::Create) {
                ContainerChangeEntry* pEntry = getNewEntry();
                pEntry->uiEntryDesc   = entry->uiEntryDesc; //ContainerChangeEntry::Change; //TODO: check what I did here (workaround to get created entries into the changelist aswell)
                pEntry->pFieldFlags   = entry->pFieldFlags;
                pEntry->uiContainerId = entry->uiContainerId;
                pEntry->whichField    = entry->whichField;
                if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                    pEntry->whichField |= *entry->bvUncommittedChanges;
                pEntry->pList         = this;
            } else*/

            if (entry->uiEntryDesc == ContainerChangeEntry::Change) {
                ContainerChangeEntry* pEntry = 0;
                if (changedFCs.count(entry->uiContainerId)) pEntry = changedFCs[entry->uiContainerId];
                else {
                    pEntry = getNewEntry();
                    pEntry->uiContainerId = entry->uiContainerId;
                    changedFCs[pEntry->uiContainerId] = pEntry;
                }
                pEntry->uiEntryDesc   = entry->uiEntryDesc; //ContainerChangeEntry::Change; //TODO: check what I did here (workaround to get created entries into the changelist aswell)
                //pEntry->pFieldFlags   = entry->pFieldFlags; // what are they used for?
                pEntry->whichField |= entry->whichField;
                if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                    pEntry->whichField |= *entry->bvUncommittedChanges;
                pEntry->whichField &= mask;
                pEntry->pList         = this;
            }
        }

        ContainerChangeEntry* newCreate(UInt32 ID, BitVector fields) {
            auto entry = getNewCreatedEntry();
            entry->uiEntryDesc = ContainerChangeEntry::Create;
            entry->uiContainerId = ID;
            entry->whichField = fields;
            return entry;
        }

        void addCreate(ContainerChangeEntry* entry) {
            ContainerChangeEntry* pEntry = getNewCreatedEntry();
            pEntry->uiEntryDesc   = entry->uiEntryDesc;
            pEntry->uiContainerId = entry->uiContainerId;
            pEntry->whichField    = entry->whichField;
            if (pEntry->whichField == 0 && entry->bvUncommittedChanges != 0)
                pEntry->whichField |= *entry->bvUncommittedChanges;
            pEntry->pList         = this;
        }
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) {
//            cout << "ourBinaryDataHandler -> read() data.size" << data.size() << " size " << size << endl;
            size_t readSize = min((size_t)size, data.size()-progress);
            memcpy(src, &data[progress], readSize); //read data from handler into src (sentry.fieldMask)
            progress += readSize;
        }

        void write(MemoryHandle src, UInt32 size) {
            data.insert(data.end(), src, src + size);
        }

        vector<unsigned char> data;
        size_t progress = 0;
};

void debugBinary(FieldContainer* fcPtr, ourBinaryDataHandler& handler2, SerialEntry& sentry) {
    //auto core = dynamic_cast<Node*>(fcPtr)->getCore();
    auto ccoreSF = dynamic_cast<Node*>(fcPtr)->getSFCore();
    auto coreSF = const_cast<SFUnrecChildNodeCorePtr*>(ccoreSF);

    ourBinaryDataHandler handler;
    fcPtr->copyToBin(handler, sentry.fieldMask);
    fcPtr->copyFromBin(handler, sentry.fieldMask);
    coreSF->copyFromBin(handler);

    cout << "\n------------------------- debugBinary \n";
    cout << "node: " << fcPtr->getId() << endl; // << ", coreID: " << core->getId() << endl;
    cout << " data size: " << handler.data.size() << endl;
    for (UInt32 i=0; i<handler.data.size(); i++) {
        cout << i << " \t " << (UInt32)handler.data[i] << endl;
    }
    cout << "------------------------- \n";
}

struct VRSyncNodeFieldContainerMapper : public ContainerIdMapper {
    VRSyncNode* syncNode = 0;

    UInt32 map(UInt32 uiId) const;
    VRSyncNodeFieldContainerMapper(VRSyncNode* node) : syncNode(node) {};

 //   private:
 //   VRSyncNodeFieldContainerMapper( const VRSyncNodeFieldContainerMapper &other);
 //   void operator =(const VRSyncNodeFieldContainerMapper &other);
};

UInt32 VRSyncNodeFieldContainerMapper::map(UInt32 uiId) const {
    UInt32 id = syncNode ? syncNode->getRemoteToLocalID(uiId) : 0;
    if (id == 0) {
        cout << " --- WARNING in VRSyncNodeFieldContainerMapper::map remote id " << uiId << " to " << id << ", syncNode: " << syncNode->getName() << endl;
        if (syncNode) syncNode->broadcast("warn|mappingFailed|"+toString(uiId));
    }
    return id;
}

OSGChangeList* VRSyncChangelist::filterChanges(VRSyncNodePtr syncNode) {
    // go through all changes, gather changes where the container is known (in containers)
    // create local changelist with changes of containers of the subtree of this sync node :D
    ThreadRefPtr applicationThread = dynamic_cast<Thread*>(ThreadManager::getAppThread());
    ChangeList* cl = applicationThread->getChangeList();
//    cout << "cl entries: " << cl->getNumChanged() + cl->getNumCreated() << endl;
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return 0;

    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    /*if (cl->getNumChanged() + cl->getNumCreated() >= 1) {
        ContainerChangeEntry* entry = *cl->begin();
        cout << " entry: " << entry->uiContainerId << " " << factory->getContainer(entry->uiContainerId)->getTypeName() << " " << entry->whichField << endl;
        cout << " Node::CoreFieldMask " << Node::CoreFieldMask << endl;
    }*/

    /*if (cl->getNumChanged() < 15) {
        cout << " GLOBALE CHANGES:" << endl;
        printChangeList((OSGChangeList*)cl);
    }*/

    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    map<UInt32, ContainerChangeEntry*> changedFCs;

    // register created and add them to local CL
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;

        FieldContainer* fct = factory->getContainer(id);
        if (fct) {
            auto type = factory->findType(fct->getTypeId());
            Attachment* att = dynamic_cast<Attachment*>(fct);
            if (id > 3014 && !syncNode->isSubContainer(id)) {
                if (att) {
                    //auto parents = att->getMFParents();
                    //cout << " ----- getFilteredChangeList entry: " << id << " " << fct->getTypeName() << " isNode? " << type->isNode() << " isCore? " << type->isNodeCore() << " isAttachment? " << type->isAttachment() << " Nparents: " << parents->size() << endl;

                }
                //cout << " ----- getFilteredChangeList entry: " << id << " " << fct->getTypeName() << " isNode? " << type->isNode() << " isCore? " << type->isNodeCore() << " isAttachment? " << type->isAttachment() << endl;
                if (type->isNode()) {
                    //cout << "    node name: " << ::getName((Node*)fct) << endl;
                }
            }
            //if (!att && !type->isNode() && !type->isNodeCore()) cout << " ----- getFilteredChangeList entry: " << fct->getTypeName() << "  " << fct->getId() << endl;
            //if (fct) cout << " getFilteredChangeList entry: " << fct->getTypeName() << " attachment? " << att << endl;
            //if (att) cout << "    attachement N parents: " << att->getMFParents()->size() << endl;
        }

        if (syncNode->isRemoteChange(id)) continue;

        if (syncNode->isSubContainer(id)) {
            localChanges->addCreate(entry);
            //cout << "    isSubContainer: " << id << " " << container.size() << endl;
            syncNode->registerContainer(factory->getContainer(id), syncNode->getContainerCount());
        }
    }

    //bool childEvent = false;

    // add changed entries to local CL
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;
        if (syncNode->isRemoteChange(id)) {
            //cout << "ignore remote change " << id << endl;
            continue;
        }

        if (syncNode->isRegistered(id)) localChanges->addChange(entry, changedFCs);
        UInt32 cMask;
        if (syncNode->isExternalContainer(id, cMask)) localChanges->addChange(entry, changedFCs, cMask);
        if (!syncNode->isSubContainer(id)) continue;

        // now check if the container is a node and if his core or children contain unregistered nodes

        // get changes fieldmask
        BitVector whichField = entry->whichField;
        if (whichField == 0 && entry->bvUncommittedChanges != 0) whichField |= *entry->bvUncommittedChanges;
//        cout << "entry->whichField " << entry->whichField << endl;

        // get container
        FieldContainer* fc = factory->getContainer(id);
        if (!fc) continue;
        Node* node = dynamic_cast<Node*>(fc);
        if (!node) continue;

        // check for unregistered containers
        if (whichField & Node::ChildrenFieldMask || whichField & Node::CoreFieldMask) {
            //childEvent = true;
            auto subcontainers = syncNode->getAllSubContainers( node );
            for (auto subc : subcontainers) {
                localChanges->newCreate(subc.first->getId(), 0);
                localChanges->newChange(subc.first->getId(), -1, changedFCs);
                syncNode->registerContainer(subc.first, syncNode->getContainerCount());
                for (auto subcParent : subc.second) {
                    if (subcParent) {
                        localChanges->newChange(subcParent->getId(), -1, changedFCs);
                    }
                }
            }
        }
    }

    //if (childEvent) printChangeList(localChanges);
    if (localChanges) {
        if (localChanges->getNumCreated() == 0 && localChanges->getNumChanged() == 0) return 0;
        else cout <<  "  local changelist, created: " << localChanges->getNumCreated() << ", changes: " << localChanges->getNumChanged() << endl;
    }

    return localChanges;
}

static vector<FieldContainerRecPtr> debugStorage;

FieldContainerRecPtr VRSyncChangelist::getOrCreate(VRSyncNodePtr syncNode, UInt32& id, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren) {
    //cout << "VRSyncNode::getOrCreate remote: " << sentry.localId << ", local: " << id << endl;
    FieldContainerRecPtr fcPtr = 0; // Field Container to apply changes to
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    if (id != 0) {
        fcPtr = factory->getContainer(id);
    } else if (sentry.uiEntryDesc == ContainerChangeEntry::Create) {
        auto tID = syncNode->getLocalType(sentry.fcTypeID);
        FieldContainerType* fcType = factory->findType(tID);
        if (fcType == 0) {
            cout << "Error in VRSyncChangelist::getOrCreate, unknown FC type!";
            cout << " remote container ID : " << sentry.localId << ", remote type ID : " << sentry.fcTypeID << endl;
            syncNode->broadcast("warn|unknownType "+ toString(sentry.fcTypeID) +"|" + toString(sentry.localId));
            return 0;
        }
        fcPtr = fcType->createContainer();
        justCreated.push_back(fcPtr); // increase ref count temporarily to avoid destruction!
        //debugStorage.push_back(fcPtr); // increase ref count permanently to avoid destruction! only for testing!
        syncNode->registerContainer(fcPtr.get(), sentry.syncNodeID);
        id = fcPtr.get()->getId();
        syncNode->addRemoteMapping(id, sentry.localId);
        //cout << " ---- create, new ID, remote: " << sentry.localId << ", local: " << id << endl;
    }
    //cout << " VRSyncNode::getOrCreate done with " << fcPtr->getId() << endl;
    return fcPtr;
}

void VRSyncChangelist::handleChildrenChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren) {
    //cout << "VRSyncNode::handleChildrenChange +++++++++++++++++++++++++++++++++++++++++++++++ " << fcPtr->getId() << endl;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    vector<UInt32> childrenIDs = parentToChildren[sentry.localId];
    //cout << " N children: " << childrenIDs.size() << endl;
    for (auto cID : childrenIDs) {
        UInt32 childID = syncNode->getRemoteToLocalID(cID);
        FieldContainer* childPtr = factory->getContainer(childID);
        Node* child = dynamic_cast<Node*>(childPtr);
        cout << "  child: " << childID << " " << child << endl;
        if (!child) continue;
        node->addChild(child);
        cout << " add child, parent: " << node->getId() << ", child: " << child->getId() << " ------------------- " << endl;
    }
    //cout << "  VRSyncNode::handleChildrenChange done" << endl;
}

void VRSyncChangelist::handleCoreChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry) {
    //cout << "VRSyncNode::handleCoreChange" << endl;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    UInt32 coreID = syncNode->getRemoteToLocalID(sentry.coreID);
    FieldContainer* corePtr = factory->getContainer(coreID);
    NodeCore* core = dynamic_cast<NodeCore*>(corePtr);
    if (!core) return;
    node->setCore(core);
    //cout << " VRSyncNode::handleCoreChange done" << endl;
}

void VRSyncChangelist::fixNullChildren(FieldContainerRecPtr fcPtr, UInt32 fieldMask) {
    if (!(fieldMask & Node::ChildrenFieldMask)) return;
    if (!fcPtr->getType().isNode()) return;
    NodeRecPtr node = dynamic_pointer_cast<Node>(fcPtr);
    vector<size_t> toRemove;
    for (size_t i=0; i<node->getNChildren(); i++) {
        if (node->getChild(i) == 0) toRemove.push_back(i);
    }
    for (auto i : toRemove) node->subChild(i);
}

void VRSyncChangelist::fixNullCore(FieldContainerRecPtr fcPtr, UInt32 fieldMask) {
    if (!(fieldMask & Node::CoreFieldMask)) return;
    if (!fcPtr->getType().isNode()) return;
    NodeRecPtr node = dynamic_pointer_cast<Node>(fcPtr);
    if (!node->getCore()) node->setCore(Group::create());
}

void VRSyncChangelist::handleGenericChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<unsigned char>>& fcData) {
    //cout << " VRSyncChangelist::handleGenericChange " << sentry.localId << " -> " << fcPtr->getId() << endl;}
    vector<unsigned char>& FCdata = fcData[sentry.localId];
    ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer
    handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata

    fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read
    fixNullChildren(fcPtr, sentry.fieldMask);
    fixNullCore(fcPtr, sentry.fieldMask);
    auto obj = syncNode->getVRObject(fcPtr->getId());
    if (obj) obj->wrapOSG(obj->getNode()); // update VR Objects, for example the VRTransform after its Matrix changed!
}

void VRSyncChangelist::handleRemoteEntries(VRSyncNodePtr syncNode, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData) {
    for (auto sentry : entries) {
        //cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << " syncID " << sentry.syncNodeID << " at pos " << pos << endl;

        //sync of initial syncNode container
        if (sentry.syncNodeID >= 0 && sentry.syncNodeID <= 2) {
            syncNode->replaceContainerMapping(sentry.syncNodeID, sentry.localId);
        }

        UInt32 id = syncNode->getRemoteToLocalID(sentry.localId);// map remote id to local id if exist (otherwise id = -1)
        //cout << " --- getRemoteToLocalID: " << sentry.localId << " to " << id << " syncNode: " << syncNode->getName() << ", syncNodeID: " << sentry.syncNodeID << endl;
        FieldContainerRecPtr fcPtr = getOrCreate(syncNode, id, sentry, parentToChildren); // Field Container to apply changes to

        if (fcPtr == nullptr) {
            cout << " -- WARNING in handleRemoteEntries, no container found with id " << id << ", entry local ID " << sentry.localId << endl;
            continue;
        }

        if (sentry.uiEntryDesc == ContainerChangeEntry::Change) { //if its a node change, update child info has changed. TODO: check only if children info has changed
            handleGenericChange(syncNode, fcPtr, sentry, fcData);
        }

        syncNode->logSyncedContainer(id);
    }

    // send the ID mapping of newly created field containers back to remote sync node
    if (justCreated.size() > 0) {
        size_t counter = 0;
        while (counter < justCreated.size()) {
            string mappingData = "mapping";
            for (int i=0; i<1000 && counter < justCreated.size(); i++) {
                auto fc = justCreated[counter];
                UInt32 lID = syncNode->getLocalToRemoteID(fc->getId());
                mappingData += "|" + toString(lID) + ":" + toString(fc->getId());
                counter++;
            }
            syncNode->broadcast(mappingData);
        }
        justCreated.clear();
    }
}

void VRSyncChangelist::printDeserializedData(vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData) {
    cout << " Deserialized entries:" << endl;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    for (auto entry : entries) {
        cout << "  entry: " << entry.localId;

        UInt32 noID = UInt32(-1);

        if (entry.fcTypeID != noID) {
            FieldContainerType* fcType = factory->findType(entry.fcTypeID);
            cout << ", fcType: " << fcType->getName();
        }

        if (entry.syncNodeID != noID) cout << ", syncNodeID: " << entry.syncNodeID;
        if (entry.uiEntryDesc != noID) cout << ", change type: " << getChangeType(entry.uiEntryDesc);
        if (entry.coreID != noID) cout << ", coreID: " << entry.coreID;


        if (parentToChildren[entry.localId].size() > 0) {
            cout << ", children: (";
            for (UInt32 c : parentToChildren[entry.localId]) cout << " " << c;
            cout << " )";
        }
        cout << endl;
    }
}

void VRSyncChangelist::deserializeEntries(vector<unsigned char>& data, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData) {
    UInt32 pos = 0;

    cout << " deserializeEntries, data size: " << data.size() << " -> " << CLdata.size() << endl;

    //deserialize and collect change and create entries
    while (pos + sizeof(SerialEntry) < CLdata.size()) {
        SerialEntry sentry = *((SerialEntry*)&CLdata[pos]);
        entries.push_back(sentry);

        pos += sizeof(SerialEntry);
        vector<unsigned char>& FCdata = fcData[sentry.localId];
        FCdata.insert(FCdata.end(), CLdata.begin()+pos, CLdata.begin()+pos+sentry.len);
        pos += sentry.len;

        vector<unsigned char> childrenData;
        childrenData.insert(childrenData.end(), CLdata.begin()+pos, CLdata.begin()+pos+sentry.cplen*sizeof(UInt32));
        if (childrenData.size() > 0) {
            UInt32* castedData = (UInt32*)&childrenData[0];
            for (UInt32 i=0; i<sentry.cplen; i++) {
                UInt32 childID = castedData[i];
                parentToChildren[sentry.localId].push_back(childID);
            }
        }

        pos += sentry.cplen * sizeof(UInt32);
    }
}

void VRSyncChangelist::deserializeAndApply(VRSyncNodePtr syncNode) {
    if (CLdata.size() == 0) return;
    cout << endl << "> > >  " << syncNode->getName() << " VRSyncNode::deserializeAndApply(), received data size: " << CLdata.size() << endl;
    VRSyncNodeFieldContainerMapper mapper(syncNode.get());
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    factory->setMapper(&mapper);

    map<UInt32, vector<UInt32>> parentToChildren; //maps parent ID to its children syncIDs
    vector<SerialEntry> entries;
    map<UInt32, vector<unsigned char>> fcData; // map entry localID to its binary field data

    deserializeEntries(CLdata, entries, parentToChildren, fcData);
    cout << " deserialized " << entries.size() << " entries" << endl;
    //printDeserializedData(entries, parentToChildren, fcData);
    handleRemoteEntries(syncNode, entries, parentToChildren, fcData);
    //printRegistredContainers();
    syncNode->wrapOSG();

    //exportToFile(getName()+".osg");

    factory->setMapper(0);
    cout << "            / " << syncNode->getName() << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;

    //*(UInt32*)0=0; // induce segfault!
    CLdata.clear();
}

void VRSyncChangelist::gatherChangelistData(VRSyncNodePtr syncNode, string& data) {
    if (data.size() == 0) return;
    cout << "  gatherChangelistData " << data.size() << endl;
    auto d = VRSyncConnection::base64_decode(data);
    CLdata.insert(CLdata.end(), d.begin(), d.end());
}

void VRSyncChangelist::printChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl) {
    if (!cl) return;
    cout << endl << "ChangeList:";
    if (cl->getNumChanged() == 0 && cl->getNumCreated() == 0) cout << " no changes " << endl;
    else cout << " node: " << syncNode->getName() << ", changes: " << cl->getNumChanged() << ", created: " << cl->getNumCreated() << endl;

    auto printEntry = [&](ContainerChangeEntry* entry) {
        //const FieldFlags* fieldFlags = entry->pFieldFlags;
        BitVector whichField = entry->whichField;
        if (whichField == 0 && entry->bvUncommittedChanges != 0) whichField |= *entry->bvUncommittedChanges;
        UInt32 id = entry->uiContainerId;

        // ----- print info ---- //
        string type;
        FieldContainerFactoryBase* factory = FieldContainerFactory::the();
        if (factory->getContainer(id)) type = factory->getContainer(id)->getTypeName();
        string changeType = getChangeType(entry->uiEntryDesc);
        cout << "  " << "uiContainerId: " << id << ", changeType: " << changeType << ", container: " << type;
        cout << ", fields: " << std::bitset<64>(whichField);
        //cout << ", node core changed? " << bool(whichField & Node::CoreFieldMask);
        //cout << ", node children changed? " << bool(whichField & Node::ChildrenFieldMask);
        cout << endl;
    };

    cout << " Created:" << endl;
    for (auto it = cl->beginCreated(); it != cl->endCreated(); ++it) printEntry(*it);
    cout << " Changes:" << endl;
    for (auto it = cl->begin(); it != cl->end(); ++it) printEntry(*it);
}

string VRSyncChangelist::getChangeType(UInt32 uiEntryDesc) {
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



vector<UInt32> VRSyncChangelist::getFCChildren(FieldContainer* fcPtr, BitVector fieldMask) {
    vector<UInt32> res;
    Node* node = dynamic_cast<Node*>(fcPtr);
    if (fieldMask & Node::ChildrenFieldMask) { // new child added
        for (UInt32 i=0; i<node->getNChildren(); i++) {
            Node* child = node->getChild(i);
            res.push_back(child->getId());
            //UInt32 syncID = container[child->getId()] ? container[child->getId()] : -1;
            //res.push_back(make_pair(child->getId(), syncID));
            //cout << "VRSyncChangelist::getFCChildren  children.push_back " << child->getId() << endl; //Debugging
        }
    }
    return res;
}

void VRSyncChangelist::filterFieldMask(VRSyncNodePtr syncNode, FieldContainer* fc, SerialEntry& sentry) {
    if (sentry.localId == syncNode->getNode()->node->getId()) { // check for sync node ID
        sentry.fieldMask &= ~Node::ParentFieldMask; // remove parent field change!
        sentry.fieldMask &= ~Node::CoreFieldMask; // remove core field change!
    }

    //FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    if (dynamic_cast<Geometry*>(fc)) {
        sentry.fieldMask &= ~Geometry::ClassicGLIdFieldMask;
        sentry.fieldMask &= ~Geometry::AttGLIdFieldMask;
        sentry.fieldMask &= ~Geometry::ClassicVaoGLIdFieldMask;
        sentry.fieldMask &= ~Geometry::AttribVaoGLIdFieldMask;
    }

    if (dynamic_cast<Surface*>(fc)) {
        sentry.fieldMask &= ~Surface::SurfaceGLIdFieldMask;
    }

    if (dynamic_cast<GeoProperty*>(fc)) {
        sentry.fieldMask &= ~GeoProperty::GLIdFieldMask;
    }

    if (dynamic_cast<GeoMultiPropertyData*>(fc)) {
        sentry.fieldMask &= ~GeoMultiPropertyData::GLIdFieldMask;
    }

    if (dynamic_cast<RenderBuffer*>(fc)) {
        sentry.fieldMask &= ~RenderBuffer::GLIdFieldMask;
    }

    if (dynamic_cast<FrameBufferObject*>(fc)) {
        sentry.fieldMask &= ~FrameBufferObject::GLIdFieldMask;
        sentry.fieldMask &= ~FrameBufferObject::MultiSampleGLIdFieldMask;
    }

    if (dynamic_cast<TextureObjRefChunk*>(fc)) {
        sentry.fieldMask &= ~TextureObjRefChunk::OsgGLIdFieldMask;
        sentry.fieldMask &= ~TextureObjRefChunk::OglGLIdFieldMask;
    }

    if (dynamic_cast<UniformBufferObjStd140Chunk*>(fc)) {
        sentry.fieldMask &= ~UniformBufferObjStd140Chunk::GLIdFieldMask;
    }

    if (dynamic_cast<ShaderStorageBufferObjStdLayoutChunk*>(fc)) {
        sentry.fieldMask &= ~ShaderStorageBufferObjStdLayoutChunk::GLIdFieldMask;
    }

    if (dynamic_cast<TextureObjChunk*>(fc)) {
        sentry.fieldMask &= ~TextureObjChunk::GLIdFieldMask;
    }

    if (dynamic_cast<UniformBufferObjChunk*>(fc)) {
        sentry.fieldMask &= ~UniformBufferObjChunk::GLIdFieldMask;
    }

    if (dynamic_cast<ShaderStorageBufferObjChunk*>(fc)) {
        sentry.fieldMask &= ~ShaderStorageBufferObjChunk::GLIdFieldMask;
    }

    if (dynamic_cast<ShaderExecutableChunk*>(fc)) {
        sentry.fieldMask &= ~ShaderExecutableChunk::GLIdFieldMask;
    }

    if (dynamic_cast<SimpleSHLChunk*>(fc)) {
        sentry.fieldMask &= ~SimpleSHLChunk::GLIdFieldMask;
    }

    if (dynamic_cast<ShaderProgram*>(fc)) {
        sentry.fieldMask &= ~ShaderProgram::GLIdFieldMask;
    }

    if (dynamic_cast<ProgramChunk*>(fc)) {
        sentry.fieldMask &= ~ProgramChunk::GLIdFieldMask;
    }
}

void VRSyncChangelist::serialize_entry(VRSyncNodePtr syncNode, ContainerChangeEntry* entry, vector<unsigned char>& data, UInt32 syncNodeID) {
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainer* fcPtr = factory->getContainer(entry->uiContainerId);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = entry->uiContainerId;
        sentry.fieldMask = entry->whichField;
        sentry.syncNodeID = syncNodeID;
        sentry.uiEntryDesc = entry->uiEntryDesc;
        sentry.fcTypeID = fcPtr->getTypeId();

        filterFieldMask(syncNode, fcPtr, sentry);

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        //if (fcPtr->getId() == 3024 && sentry.uiEntryDesc == ContainerChangeEntry::Change)
        //    debugBinary(fcPtr, handler);

        vector<UInt32> children;
        if (factory->findType(sentry.fcTypeID)->isNode()) children = getFCChildren(fcPtr, sentry.fieldMask);
        sentry.cplen = children.size();

        if (factory->findType(sentry.fcTypeID)->isNode()) {
            Node* node = dynamic_cast<Node*>(fcPtr);
            sentry.coreID = node->getCore()->getId();
//            if (sentry.fieldMask & Node::CoreFieldMask) { // node core changed
//                sentry.coreID = node->getCore()->getId();
//            }
        }

        data.insert(data.end(), (unsigned char*)&sentry, (unsigned char*)&sentry + sizeof(SerialEntry));
        data.insert(data.end(), handler.data.begin(), handler.data.end());

        if (sentry.cplen > 0) data.insert(data.end(), (unsigned char*)&children[0], (unsigned char*)&children[0] + sizeof(UInt32)*sentry.cplen);
    }
}

string VRSyncChangelist::serialize(VRSyncNodePtr syncNode, ChangeList* clist) {
    cout << "> > >  " << syncNode->getName() << " VRSyncNode::serialize()" << endl; //Debugging

    vector<unsigned char> data;
    size_t i = 0;

    for (auto it = clist->beginCreated(); it != clist->endCreated(); it++, i++) {
        ContainerChangeEntry* entry = *it;
        serialize_entry(syncNode, entry, data, syncNode->getContainerMappedID(entry->uiContainerId));
    }

    for (auto it = clist->begin(); it != clist->end(); it++, i++) {
        ContainerChangeEntry* entry = *it;
        serialize_entry(syncNode, entry, data, syncNode->getContainerMappedID(entry->uiContainerId));
    }

    cout << "serialized entries: " << i << endl; //Debugging
    cout << "            / " << syncNode->getName() << " / VRSyncNode::serialize(), data size: " << data.size() << "  < < <" << endl; //Debugging

    if (data.size() == 0) return "";
    return VRSyncConnection::base64_encode(&data[0], data.size());
}

void VRSyncChangelist::broadcastChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl, bool doDelete) {
    if (!cl) return;
    string data = serialize(syncNode, cl); // serialize changes in new change list (check OSGConnection for serialization Implementation)
    syncNode->broadcast(data); // send over websocket to remote
    syncNode->broadcast("changelistEnd|");
    if (doDelete) delete cl;
}

//copies state into a CL and serializes it as string
void VRSyncChangelist::broadcastSceneState(VRSyncNodePtr syncNode) {
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    localChanges->fillFromCurrentState();
    broadcastChangeList(syncNode, localChanges, true);
}





