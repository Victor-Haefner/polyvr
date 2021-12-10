#include "VRSyncChangelist.h"
#include "VRSyncNode.h"
#include "core/objects/OSGObject.h"
#include "core/utils/toString.h"
#include "core/gui/VRGuiConsole.h"

#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGFieldContainer.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGAttachment.h>
#include <OpenSG/OSGStringAttributeMap.h>
#include <OpenSG/OSGNameAttachment.h>
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
            if (entry->uiEntryDesc == ContainerChangeEntry::AddReference   ||
                entry->uiEntryDesc == ContainerChangeEntry::SubReference   ||
                entry->uiEntryDesc == ContainerChangeEntry::DepSubReference) {
                ContainerChangeEntry* pEntry = getNewEntry();
                pEntry->uiEntryDesc   = entry->uiEntryDesc;
                pEntry->uiContainerId = entry->uiContainerId;
                pEntry->pList         = this;
            }

            if (entry->uiEntryDesc == ContainerChangeEntry::Change) {
                ContainerChangeEntry* pEntry = 0;
                if (changedFCs.count(entry->uiContainerId)) pEntry = changedFCs[entry->uiContainerId];
                else {
                    pEntry = getNewEntry();
                    pEntry->uiContainerId = entry->uiContainerId;
                    changedFCs[pEntry->uiContainerId] = pEntry;
                }
                pEntry->uiEntryDesc = entry->uiEntryDesc; //ContainerChangeEntry::Change; //TODO: check what I did here (workaround to get created entries into the changelist aswell)
                //pEntry->pFieldFlags   = entry->pFieldFlags; // what are they used for?
                pEntry->whichField |= entry->whichField;
                if (entry->bvUncommittedChanges != 0) pEntry->whichField |= *entry->bvUncommittedChanges;
                pEntry->whichField &= mask;
                pEntry->pList = this;
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
            if (entry->bvUncommittedChanges != 0) pEntry->whichField |= *entry->bvUncommittedChanges;
            pEntry->pList         = this;
        }
};

class ourBinaryDataHandler : public BinaryDataHandler {
    public:
        ourBinaryDataHandler() {
            forceDirectIO();
        }

        void read(MemoryHandle src, UInt32 size) override {
//            cout << "ourBinaryDataHandler -> read() data.size" << data.size() << " size " << size << endl;
            size_t readSize = min((size_t)size, data.size()-progress);
            memcpy(src, &data[progress], readSize); //read data from handler into src (sentry.fieldMask)
            progress += readSize;
        }

        void write(MemoryHandle src, UInt32 size) override {
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
    VRSyncConnectionWeakPtr weakRemote;

    UInt32 map(UInt32 uiId) const override;
    VRSyncNodeFieldContainerMapper(VRSyncNode* node, VRSyncConnectionWeakPtr weakRemote) : syncNode(node), weakRemote(weakRemote) {};

 //   private:
 //   VRSyncNodeFieldContainerMapper( const VRSyncNodeFieldContainerMapper &other);
 //   void operator =(const VRSyncNodeFieldContainerMapper &other);
};

static map<UInt32, int> blacklist;

UInt32 VRSyncNodeFieldContainerMapper::map(UInt32 uiId) const {
    UInt32 id = 0;

    if (syncNode) {
        auto remote = weakRemote.lock();
        if (remote) id = remote->getLocalID(uiId);
    }

    if (id == 0) {
        if (blacklist[id] < 10) {
            blacklist[id]++;

            if (syncNode) {
                if (syncNode) cout << " --- WARNING in VRSyncNodeFieldContainerMapper::map remote id " << uiId << " to " << id << ", syncNode: " << syncNode->getName() << endl;
                auto remote = weakRemote.lock();
                if (remote) remote->send("warn|failed fc pointer mapping|"+toString(uiId));
            }

#ifndef WITHOUT_GTK
            VRConsoleWidget::get("Collaboration")->write( " Warning in sync FC mapper, could not map "+toString(uiId)+"\n", "red");
#endif
        }
    }
    return id;
}

OSGChangeList* VRSyncChangelist::filterChangeList(VRSyncNodePtr syncNode, ChangeList* cl) {
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    if (cl->getNumCreated() > 0) {
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( "Filter Changelist with "+toString(cl->getNumCreated())+" created FCs\n");
#endif
        /*ContainerChangeEntry* entry = *cl->begin();
        cout << " entry: " << entry->uiContainerId << " " << factory->getContainer(entry->uiContainerId)->getTypeName() << " " << entry->whichField << endl;
        cout << " Node::CoreFieldMask " << Node::CoreFieldMask << endl;*/
    }

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

        for (auto rID : syncNode->getRemotes())
            if (auto remote = syncNode->getRemote(rID))
                if (remote->isRemoteChange(id)) continue;

        if (syncNode->isSubContainer(id)) {
#ifndef WITHOUT_GTK
            VRConsoleWidget::get("Collaboration")->write( " add created FC "+toString(id));
            if (fct) VRConsoleWidget::get("Collaboration")->write( " of type "+string(fct->getTypeName())+"\n");
            else VRConsoleWidget::get("\n");
#endif

            localChanges->addCreate(entry);
            //cout << "    isSubContainer: " << id << " " << container.size() << endl;
            syncNode->registerContainer(factory->getContainer(id));
        }
    }

    //bool childEvent = false;

    // add changed entries to local CL
    for (auto it = cl->begin(); it != cl->end(); ++it) {
        ContainerChangeEntry* entry = *it;
        UInt32 id = entry->uiContainerId;

        for (auto rID : syncNode->getRemotes())
            if (auto remote = syncNode->getRemote(rID))
                if (remote->isRemoteChange(id)) continue;

        if (syncNode->isRegistered(id)) localChanges->addChange(entry, changedFCs);

        UInt32 cMask;
        if (syncNode->isExternalContainer(id, cMask)) localChanges->addChange(entry, changedFCs, cMask);

        if (!syncNode->isSubContainer(id)) continue;

        // now check if the container is a node and if his core or children contain unregistered nodes

        // get changes fieldmask
        BitVector whichField = entry->whichField;
        if (entry->bvUncommittedChanges != 0) whichField |= *entry->bvUncommittedChanges;
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
                syncNode->registerContainer(subc.first);
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
        if (localChanges->getNumCreated() > 0 || localChanges->getNumChanged() > 2)
            cout <<  "  local changelist, created: " << localChanges->getNumCreated() << ", changes: " << localChanges->getNumChanged() << endl;
    }

    return localChanges;
}

OSGChangeList* VRSyncChangelist::filterChanges(VRSyncNodePtr syncNode) {
    // go through all changes, gather changes where the container is known (in containers)
    // create local changelist with changes of containers of the subtree of this sync node :D
    ThreadRefPtr applicationThread = dynamic_cast<Thread*>(ThreadManager::getAppThread());
    ChangeList* cl = applicationThread->getChangeList();
//    cout << "cl entries: " << cl->getNumChanged() + cl->getNumCreated() << endl;
    if (cl->getNumChanged() + cl->getNumCreated() == 0) return 0;

    return filterChangeList(syncNode, cl);
}

static vector<FieldContainerRecPtr> debugStorage;

FieldContainerRecPtr VRSyncChangelist::getOrCreate(VRSyncNodePtr syncNode, UInt32& id, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren, VRSyncConnectionWeakPtr weakRemote) {
    auto remote = weakRemote.lock();
    if (!remote) return 0;

    //cout << "VRSyncNode::getOrCreate remote: " << sentry.localId << ", local: " << id << endl;
    FieldContainerRecPtr fcPtr = 0; // Field Container to apply changes to
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    if (id != 0) {
        fcPtr = factory->getContainer(id);
    } else if (sentry.uiEntryDesc == ContainerChangeEntry::Create) {
        auto tID = remote->getLocalType(sentry.fcTypeID);
        FieldContainerType* fcType = factory->findType(tID);
        if (fcType == 0) {
            cout << "Error in VRSyncChangelist::getOrCreate, unknown FC type!";
            cout << " remote container ID : " << sentry.localId << ", remote type ID : " << sentry.fcTypeID << endl;
            syncNode->broadcast("warn|unknownType "+ toString(sentry.fcTypeID) +"|" + toString(sentry.localId));
#ifndef WITHOUT_GTK
            VRConsoleWidget::get("Collaboration")->write( "  Warning in sync FC access, unknown FC type ID "+toString(sentry.fcTypeID)+"\n", "red");
#endif
            return 0;
        }
        fcPtr = fcType->createContainer();
        justCreated.push_back(fcPtr); // increase ref count temporarily to avoid destruction!
        //debugStorage.push_back(fcPtr); // increase ref count permanently to avoid destruction! only for testing!
        syncNode->registerContainer(fcPtr.get());
        id = fcPtr.get()->getId();
        remote->addRemoteMapping(id, sentry.localId);
        cout << " ---- syncNode " << syncNode->getName() << ", create, new ID, remote: " << sentry.localId << ", local: " << id << ", remoteNode: " << remote->getID() << endl;
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( "  created new FC, remote ID "+toString(sentry.localId)+", local ID "+toString(id)+", type: "+fcPtr.get()->getTypeName()+"\n");
#endif
    }
    //cout << " VRSyncNode::getOrCreate done with " << fcPtr->getId() << endl;
    return fcPtr;
}

void VRSyncChangelist::handleChildrenChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren, VRSyncConnectionWeakPtr weakRemote) {
    auto remote = weakRemote.lock();
    if (!remote) return;

    //cout << "VRSyncNode::handleChildrenChange +++++++++++++++++++++++++++++++++++++++++++++++ " << fcPtr->getId() << endl;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    vector<UInt32> childrenIDs = parentToChildren[sentry.localId];
    //cout << " N children: " << childrenIDs.size() << endl;
    for (auto cID : childrenIDs) {
        UInt32 childID = remote->getLocalID(cID);
        FieldContainer* childPtr = factory->getContainer(childID);
        Node* child = dynamic_cast<Node*>(childPtr);
        //cout << "  child: " << childID << " " << child << endl;
        if (!child) {
#ifndef WITHOUT_GTK
            VRConsoleWidget::get("Collaboration")->write( " sync: add child failed, child "+toString(cID)+"/"+toString(childID)+" not found!\n", "red");
#endif
            continue;
        }
        node->addChild(child);
        //cout << " add child, parent: " << node->getId() << ", child: " << child->getId() << " ------------------- " << endl;
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( " sync: add child, parent: "+toString(node->getId())+", child: "+toString(child->getId())+"\n");
#endif
    }
    //cout << "  VRSyncNode::handleChildrenChange done" << endl;
}

void VRSyncChangelist::handleCoreChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, VRSyncConnectionWeakPtr weakRemote) {
    auto remote = weakRemote.lock();
    if (!remote) return;

    //cout << "VRSyncNode::handleCoreChange" << endl;
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainerType* fcType = factory->findType(sentry.fcTypeID);
    if (!fcType->isNode()) return;
    Node* node = dynamic_cast<Node*>(fcPtr.get());
    if (!node) return;

    UInt32 coreID = remote->getLocalID(sentry.coreID);
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
    if (node->getNChildren() == 0) return;
    vector<size_t> toRemove;
    for (size_t i=node->getNChildren()-1; i>0; i--) {
        Node* child = node->getChild(i);
        if (!child) toRemove.push_back(i);
    }
    for (auto i : toRemove) node->subChild(i);
}

void VRSyncChangelist::fixNullCore(FieldContainerRecPtr fcPtr, UInt32 fieldMask) {
    if (!(fieldMask & Node::CoreFieldMask)) return;
    if (!fcPtr->getType().isNode()) return;
    NodeRecPtr node = dynamic_pointer_cast<Node>(fcPtr);
    if (!node->getCore()) node->setCore(Group::create());
}

vector<Node*> oldChildren;
vector<Node*> newChildren;

bool isInside(Node* n, vector<Node*>& v) {
    for (int i=0; i<v.size(); i++) if (v[i] == n) return true;
    return false;
}

void VRSyncChangelist::checkChildrenChange(FieldContainerRecPtr fcPtr, UInt32 fieldMask) {
    if (!fcPtr->getType().isNode()) return;
    NodeRecPtr node = dynamic_pointer_cast<Node>(fcPtr);
    string nName = getName(node) ? getName(node) : "unnamed";

    if ((fieldMask & Node::ParentFieldMask)) {
        Node* parent = node->getParent();
        int parentID = parent ? parent->getId() : 0;
        cout << " node: " << nName << " (" << node->getId() << "), parent bevor change: " << parentID << endl;
        if (parent) {
            cout << "  parent (" << parentID << ") current children count: " << parent->getNChildren() << endl;
            for (size_t i=0; i<parent->getNChildren(); i++) {
                Node* child = parent->getChild(i);
                if (child) cout << "    child: " << getName(child) << ", " << child->getId() << endl;
                else cout << "    child: " << child << endl;

                if (child == node) { // TODO: test if using the handle avoids to add change to lists
                    parent->subChild(i);
                    //EditFieldHandlePtr h = parent->editField(Node::ChildrenFieldId);
                    //h->removeIndex(i);
                    break;
                }
            }
            cout << "  parent (" << parentID << ") new children count: " << parent->getNChildren() << endl;
            Node* parent = node->getParent();
            int parentID = parent ? parent->getId() : 0;
            cout << "   node " << nName << " (" << node->getId() << ") current parent: " << parentID << endl;
        }
    }

    /*if ((fieldMask & Node::ChildrenFieldMask)) {
        oldChildren.clear();

        cout << " node: " << nName << " (" << node->getId() << "), N children bevor changes: " << node->getNChildren() << endl;

        for (size_t i=0; i<node->getNChildren(); i++) {
            Node* child = node->getChild(i);
            if (child) {
                Node* parent = child->getParent();
                int parentID = parent ? parent->getId() : 0;
                string cName = getName(child) ? getName(child) : "unnamed";
                cout << "  child: " << cName << ", " << child->getId() << ", refcount: " << child->getRefCount() << ", parentID: " << parentID << endl;
                oldChildren.push_back( child );
            } else cout << "  child: " << child << endl;
        }
    }*/
}

void VRSyncChangelist::mergeChildrenChange(FieldContainerRecPtr fcPtr, UInt32 fieldMask) {
    if (!fcPtr->getType().isNode()) return;
    NodeRecPtr node = dynamic_pointer_cast<Node>(fcPtr);
    string nName = getName(node) ? getName(node) : "unnamed";

    if ((fieldMask & Node::ParentFieldMask)) {
        Node* parent = node->getParent();
        int parentID = parent ? parent->getId() : 0;
        cout << " node: " << nName << " (" << node->getId() << "), parent after change: " << parentID << endl;
        if (parent == node) {
            cout << " ERROR in VRSyncChangelist::mergeChildrenChange parent == node!" << endl;
            return;
        }
        if (parent) {
            cout << "  parent (" << parentID << ") current children count: " << parent->getNChildren() << endl;
            parent->addChild(node);
            cout << "  parent (" << parentID << ") new children count: " << parent->getNChildren() << endl;
            Node* parent = node->getParent();
            int parentID = parent ? parent->getId() : 0;
            cout << "   node " << nName << " (" << node->getId() << ") current parent: " << parentID << endl;
        }
    }

    /*if ((fieldMask & Node::ChildrenFieldMask)) {
        newChildren.clear();

        cout << " node: " << nName << " (" << node->getId() << "), N children after changes: " << node->getNChildren() << endl;

        for (size_t i=0; i<node->getNChildren(); i++) {
            Node* child = node->getChild(i);
            if (child) {
                Node* parent = child->getParent();
                int parentID = parent ? parent->getId() : 0;
                string cName = getName(child) ? getName(child) : "unnamed";
                cout << "  child: " << cName << ", " << child->getId() << ", refcount: " << child->getRefCount() << ", parentID: " << parentID << endl;
                newChildren.push_back( child );
            } else cout << "  child: " << child << endl;
        }

        for (int i=0; i<oldChildren.size(); i++) {
            Node* child = oldChildren[i];
            Node* parent = child->getParent();
            int parentID = parent ? parent->getId() : 0;
            string cName = getName(child) ? getName(child) : "unnamed";
            cout << "  old child: " << cName << ", " << child->getId() << ", refcount: " << child->getRefCount() << ", parentID: " << parentID << endl;
            if (isInside(child, newChildren)) continue;
            cout << "   missing it! keepin it! " << cName << endl;
            node->addChild(child);
        }
        cout << "  final child count of " << nName << " is: " << node->getNChildren() << endl;
    }*/
}

void VRSyncChangelist::handleGenericChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<unsigned char>>& fcData) {
    //cout << " VRSyncChangelist::handleGenericChange " << sentry.localId << " -> " << fcPtr->getId() << endl;}
    vector<unsigned char>& FCdata = fcData[sentry.localId];
    ourBinaryDataHandler handler; //use ourBinaryDataHandler to somehow apply binary change to fieldcontainer
    handler.data.insert(handler.data.end(), FCdata.begin(), FCdata.end()); //feed handler with FCdata

    checkChildrenChange(fcPtr, sentry.fieldMask);
    fcPtr->copyFromBin(handler, sentry.fieldMask); //calls handler->read
    fixNullChildren(fcPtr, sentry.fieldMask);
    fixNullCore(fcPtr, sentry.fieldMask);
    mergeChildrenChange(fcPtr, sentry.fieldMask);
    auto obj = syncNode->getVRObject(fcPtr->getId());
    if (obj) obj->wrapOSG(obj->getNode()); // update VR Objects, for example the VRTransform after its Matrix changed!

    string type = fcPtr->getTypeName();
    StringAttributeMap* attachment = dynamic_cast<StringAttributeMap*>(fcPtr.get());
    if (attachment) {
        auto pickable = attachment->getAttribute("pickable");
        Node* attachmentNode = dynamic_cast<Node*>(attachment->getParents(0));
        if (attachmentNode) {
            auto obj = syncNode->getVRObject(attachmentNode->getCore()->getId());
            if (obj) {
                bool b = (pickable == "yes");
#ifndef WITHOUT_GTK
                VRConsoleWidget::get("Collaboration")->write( " handleGenericChange: "+obj->getName()+" is pickable? "+pickable+"\n");
#endif
                obj->setPickable(b, false);
            }
        }
    }
}

void VRSyncChangelist::handleRemoteEntries(VRSyncNodePtr syncNode, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData, VRSyncConnectionWeakPtr weakRemote) {
    auto remote = weakRemote.lock();
    if (!remote) return;

    for (auto sentry : entries) {
        //cout << "deserialize > > > sentry: " << sentry.localId << " " << sentry.fieldMask << " " << sentry.len << " desc " << sentry.uiEntryDesc << " syncID " << sentry.syncNodeID << " at pos " << pos << endl;

        //sync of initial syncNode container
        /*if (sentry.syncNodeID > 0 && sentry.syncNodeID <= 3) {
            syncNode->replaceContainerMapping(sentry.syncNodeID, sentry.localId, weakRemote);
        }*/

        /*if (sentry.uiEntryDesc == ContainerChangeEntry::Create) {
            VRConsoleWidget::get("Collaboration")->write( "Create FC, remote ID: "+toString(sentry.localId)+"\n");
        }*/

        UInt32 id = remote->getLocalID(sentry.localId);// map remote id to local id if exist (otherwise id = -1)
        /*if (id == 0 && sentry.uiEntryDesc != ContainerChangeEntry::Create) { // ARGH, crashes ram!
            cout << " -- WARNING in handleRemoteEntries, no local ID found to remote id " << sentry.localId << endl;
            for (auto reID : syncNode->getRemotes()) { // NOPE.. doesnt work :/
                if (reID == rID) continue;
                auto remote = syncNode->getRemote(reID);
                if (remote) id = remote->getLocalID(sentry.localId);
                if (id != 0) break;
            }
            if (id != 0) {
                cout << " -- WARNING in handleRemoteEntries, found local ID " << id << " to remote id " << sentry.localId << " in another remote ID pool" << endl;
                remote->addRemoteMapping(id, sentry.localId);
            }
            continue;
        }*/

        //cout << " --- getRemoteToLocalID: " << sentry.localId << " to " << id << " syncNode: " << syncNode->getName() << ", syncNodeID: " << sentry.syncNodeID << endl;
        FieldContainerRecPtr fcPtr = getOrCreate(syncNode, id, sentry, parentToChildren, weakRemote); // Field Container to apply changes to
        if (fcPtr == nullptr) {
            cout << " -- WARNING in handleRemoteEntries, no container found with local id " << id << ", remote ID " << sentry.localId << endl;
            continue;
        }

        if (sentry.uiEntryDesc == ContainerChangeEntry::Change) { //if its a node change, update child info has changed. TODO: check only if children info has changed
            handleGenericChange(syncNode, fcPtr, sentry, fcData);
        }

        remote->logSyncedContainer(id);
    }

    // send the ID mapping of newly created field containers back to remote sync node
    if (justCreated.size() > 0) {
        size_t counter = 0;
        while (counter < justCreated.size()) {
            string mappingData = "";
            for (int i=0; i<1000 && counter < justCreated.size(); i++) {
                auto fc = justCreated[counter];
                UInt32 rID = remote->getRemoteID(fc->getId());
                mappingData += "|" + toString(rID) + ":" + toString(fc->getId());
                counter++;
            }
            remote->send("mapping"+mappingData);


            for (auto reID : syncNode->getRemotes()) {
                auto otherRemote = syncNode->getRemote(reID);
                if (otherRemote == remote) continue; // dont send to origin, only to others
                if (otherRemote) otherRemote->send("remoteMapping|"+remote->getID()+mappingData);
            }
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

    //cout << " deserializeEntries, data size: " << data.size() << " -> " << CLdata.size() << endl;
    int Ncreated = 0;

    //deserialize and collect change and create entries
    while (pos + sizeof(SerialEntry) < CLdata.size()) {
        SerialEntry sentry = *((SerialEntry*)&CLdata[pos]);
        entries.push_back(sentry);
        if (sentry.uiEntryDesc == ContainerChangeEntry::Create) Ncreated++;
        pos += sizeof(SerialEntry);

        vector<unsigned char>& FCdata = fcData[sentry.localId];
        if (sentry.len != UInt32(-1)) { // this checks if len > -1 (uint32)
            FCdata.insert(FCdata.end(), CLdata.begin()+pos, CLdata.begin()+pos+sentry.len);
        }
#ifndef WITHOUT_GTK
	else { VRConsoleWidget::get("Collaboration")->write( " Error in deserializing change entries, data length is -1\n", "red" ); }
#endif
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

    if (Ncreated > 0) {
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( " Deserialize changelist with "+toString(Ncreated)+" created FCs\n");
#endif
    }
}

void VRSyncChangelist::deserializeAndApply(VRSyncNodePtr syncNode, VRSyncConnectionWeakPtr weakRemote) {
    if (CLdata.size() == 0) return;
    bool verbose = false; //(CLdata.size() > 2);
    if (verbose) cout << endl << "> > >  " << syncNode->getName() << " VRSyncNode::deserializeAndApply(), received data size: " << CLdata.size() << endl;
    VRSyncNodeFieldContainerMapper mapper(syncNode.get(), weakRemote);
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    factory->setMapper(&mapper);

    map<UInt32, vector<UInt32>> parentToChildren; //maps parent ID to its children syncIDs
    vector<SerialEntry> entries;
    map<UInt32, vector<unsigned char>> fcData; // map entry localID to its binary field data

    deserializeEntries(CLdata, entries, parentToChildren, fcData);
    if (verbose) cout << " deserialized " << entries.size() << " entries" << endl;
    //printDeserializedData(entries, parentToChildren, fcData);
    handleRemoteEntries(syncNode, entries, parentToChildren, fcData, weakRemote);
    //printRegistredContainers();
    syncNode->wrapOSG();

    //exportToFile(getName()+".osg");

    factory->setMapper(0);
    if (verbose) cout << "            / " << syncNode->getName() << " VRSyncNode::deserializeAndApply()" << "  < < <" << endl;

    //*(UInt32*)0=0; // induce segfault!
    CLdata.clear();
}

void VRSyncChangelist::gatherChangelistData(VRSyncNodePtr syncNode, string& data) {
    if (data.size() == 0) return;
    //cout << "  gatherChangelistData, got " << data.size()/1000.0 << " kb" << endl;
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
        if (entry->bvUncommittedChanges != 0) whichField |= *entry->bvUncommittedChanges;
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

bool VRSyncChangelist::filterFieldMask(VRSyncNodePtr syncNode, FieldContainer* fc, SerialEntry& sentry) {
    //if (sentry.localId == syncNode->getSyncNodeID()) return false; // dismiss entry
    if (sentry.localId == syncNode->getSyncNameID()) return false; // dismiss entry
    if (sentry.localId == syncNode->getSyncCoreID()) return false; // dismiss entry

    if (sentry.localId == syncNode->getSyncNodeID()) { // check for sync node ID
        sentry.fieldMask &= ~Node::ParentFieldMask; // remove parent field change!
        sentry.fieldMask &= ~Node::CoreFieldMask; // remove core field change!
    }

    if (dynamic_cast<Node*>(fc)) { // ignore all changes to children array
        sentry.fieldMask &= ~Node::ChildrenFieldMask;
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

    return true;
}

void VRSyncChangelist::serialize_entry(VRSyncNodePtr syncNode, ContainerChangeEntry* entry, vector<unsigned char>& data) {
    FieldContainerFactoryBase* factory = FieldContainerFactory::the();
    FieldContainer* fcPtr = factory->getContainer(entry->uiContainerId);
    if (fcPtr) {
        SerialEntry sentry;
        sentry.localId = entry->uiContainerId;
        sentry.fieldMask = entry->whichField;
        sentry.uiEntryDesc = entry->uiEntryDesc;
        sentry.fcTypeID = fcPtr->getTypeId();


        //VRConsoleWidget::get("Collaboration")->write( " Serialize entry: "+toString(entry->uiContainerId)+"/"+toString(syncNodeID)+" "+toString(entry->whichField)+"\n");

        if (!filterFieldMask(syncNode, fcPtr, sentry)) return;

        ourBinaryDataHandler handler;
        fcPtr->copyToBin(handler, sentry.fieldMask); //calls handler->write
        sentry.len = handler.data.size();//UInt32(fcPtr->getBinSize(sentry.fieldMask));

        /*if (sentry.uiEntryDesc == ContainerChangeEntry::Create) {
            cout << " serialize create entry for fc: " << fcPtr->getId() << endl;
        }*/

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
    if (clist->getNumCreated() > 0) {
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( " Serialize changelist with "+toString(clist->getNumCreated())+" created FCs\n");
#endif
    }

    bool verbose = false;
    if (verbose) cout << "> > >  " << syncNode->getName() << " VRSyncNode::serialize()" << endl; //Debugging

    vector<unsigned char> data;
    size_t i = 0;

    for (auto it = clist->beginCreated(); it != clist->endCreated(); it++, i++) {
        ContainerChangeEntry* entry = *it;
        serialize_entry(syncNode, entry, data);
    }

    for (auto it = clist->begin(); it != clist->end(); it++, i++) {
        ContainerChangeEntry* entry = *it;
        serialize_entry(syncNode, entry, data);
    }

    if (verbose) cout << "serialized entries: " << i << endl; //Debugging
    if (verbose) cout << "            / " << syncNode->getName() << " / VRSyncNode::serialize(), data size: " << data.size() << "  < < <" << endl; //Debugging

    if (data.size() == 0) return "";
    return VRSyncConnection::base64_encode(&data[0], data.size());
    //return string((char*)&data[0], data.size());
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
#ifndef WITHOUT_GTK
    VRConsoleWidget::get("Collaboration")->write( " Broadcast scene state\n");
#endif
    auto fullState = ChangeList::create();
    fullState->fillFromCurrentState();
    auto localChanges = filterChangeList(syncNode, fullState);

    string data = serialize(syncNode, localChanges);
    syncNode->broadcast(data); // send over websocket to remote
    syncNode->broadcast("changelistEnd|");
}

void VRSyncChangelist::sendSceneState(VRSyncNodePtr syncNode, VRSyncConnectionWeakPtr weakRemote) {
    auto remote = weakRemote.lock();
    if (!remote) {
        VRConsoleWidget::get("Collaboration")->write( " Send scene state to"+remote->getID()+" failed! remote unknown!\n", "red");
        return;
    }

#ifndef WITHOUT_GTK
    VRConsoleWidget::get("Collaboration")->write( " Send scene state to "+remote->getID()+"\n");
#endif

    auto fullState = ChangeList::create();
    fullState->fillFromCurrentState();
    auto localChanges = filterChangeList(syncNode, fullState);

    cout << " syncNode: " << syncNode->getName() << ", Send scene state to "+remote->getID() << ", created: " << localChanges->getNumCreated() << endl;
    string data = serialize(syncNode, localChanges);
    remote->send(data);
    remote->send("changelistEnd|");
}




