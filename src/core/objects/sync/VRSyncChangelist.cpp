#include "VRSyncChangelist.h"
#include "VRSyncNode.h"

#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGFieldContainer.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGAttachment.h>
#include <OpenSG/OSGNode.h>

#include <bitset>

using namespace OSG;

VRSyncChangelist::VRSyncChangelist() {}
VRSyncChangelist::~VRSyncChangelist() { cout << "~VRSyncChangelist::VRSyncChangelist" << endl; }
VRSyncChangelistPtr VRSyncChangelist::create() { return VRSyncChangelistPtr( new VRSyncChangelist() ); }


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

        void addChange(ContainerChangeEntry* entry, map<UInt32, ContainerChangeEntry*>& changedFCs) {
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
            } else*/ if (entry->uiEntryDesc == ContainerChangeEntry::Change) {
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



/*void VRSyncChangelist::deserializeAndApply(string& data) {
    if (data.size() == 0) return;
    cout << endl << "> > >  " << name << " VRSyncChangelist::deserializeAndApply(), received data size: " << data.size() << endl;
    VRSyncNodeFieldContainerMapper mapper(this);
    factory->setMapper(&mapper);

    map<UInt32, vector<UInt32>> parentToChildren; //maps parent ID to its children syncIDs
    vector<SerialEntry> entries;
    map<UInt32, vector<BYTE>> fcData; // map entry localID to its binary field data

    deserializeEntries(data, entries, parentToChildren, fcData);
    printDeserializedData(entries, parentToChildren, fcData);
    handleRemoteEntries(entries, parentToChildren, fcData);
    //printRegistredContainers();
    wrapOSG();

    //exportToFile(getName()+".osg");

    factory->setMapper(0);
    cout << "            / " << name << " VRSyncChangelist::deserializeAndApply()" << "  < < <" << endl;

    //*(UInt32*)0=0; // induce segfault!
}

//copies state into a CL and serializes it as string
string VRSyncChangelist::copySceneState() {
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    localChanges->fillFromCurrentState();
    printChangeList(localChanges);

    string data = serialize(localChanges);
    delete localChanges;
    return data;
}*/

void VRSyncChangelist::broadcastChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl, bool doDelete) {
    if (!cl) return;
    string data = syncNode->serialize(cl); // serialize changes in new change list (check OSGConnection for serialization Implementation)
    if (doDelete) delete cl;
    syncNode->broadcast(data); // send over websocket to remote
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


//copies state into a CL and serializes it as string
string VRSyncChangelist::copySceneState(VRSyncNodePtr syncNode) {
    OSGChangeList* localChanges = (OSGChangeList*)ChangeList::create();
    localChanges->fillFromCurrentState();
    printChangeList(syncNode, localChanges);

    string data = syncNode->serialize(localChanges);
    delete localChanges;
    return data;
}

