#include "VRChangeList.h"

#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGVector.h>

using namespace OSG;


VRChangeList::VRChangeList() {}
VRChangeList::~VRChangeList() {}

int VRChangeList::getDestroyed() { return 0; }
int VRChangeList::getCreated() { return 0; }
int VRChangeList::getChanged() { return 0; }

size_t VRChangeList::getTotalEntities() { return totalEntites; }

void VRChangeList::update() {
    ChangeList* clist = Thread::getCurrentChangeList();
    //UInt32 Ncreated = clist->getNumCreated();
    //UInt32 Nchanged = clist->getNumChanged();


    UInt32 Ncreate = 0;
    UInt32 NaddRef = 0;
    UInt32 NsubRef = 0;
    UInt32 NdepSubRef = 0;
    UInt32 Nchange = 0;
    UInt32 NaddField = 0;
    UInt32 NsubField = 0;

    for (auto it = clist->begin(); it != clist->end(); it++) {
        ContainerChangeEntry* entry = *it;
        UInt32 desc = entry->uiEntryDesc;
        if (desc == ContainerChangeEntry::Create) Ncreate++;
        if (desc == ContainerChangeEntry::AddReference) NaddRef++;
        if (desc == ContainerChangeEntry::SubReference) NsubRef++;
        if (desc == ContainerChangeEntry::DepSubReference) NdepSubRef++;
        if (desc == ContainerChangeEntry::Change) Nchange++;
        if (desc == ContainerChangeEntry::AddField) NaddField++;
        if (desc == ContainerChangeEntry::SubField) NsubField++;
    }

    for (auto it = clist->beginCreated(); it != clist->endCreated(); it++) {
        ContainerChangeEntry* entry = *it;
        UInt32 desc = entry->uiEntryDesc;
        if (desc == ContainerChangeEntry::Create) Ncreate++;
        if (desc == ContainerChangeEntry::AddReference) NaddRef++;
        if (desc == ContainerChangeEntry::SubReference) NsubRef++;
        if (desc == ContainerChangeEntry::DepSubReference) NdepSubRef++;
        if (desc == ContainerChangeEntry::Change) Nchange++;
        if (desc == ContainerChangeEntry::AddField) NaddField++;
        if (desc == ContainerChangeEntry::SubField) NsubField++;
    }

    cout << "VRChangeList::update " << Vec3i(Ncreate, NaddRef, NsubRef) << "   " << Vec4i(NdepSubRef, Nchange, NaddField, NsubField) << endl;
}
