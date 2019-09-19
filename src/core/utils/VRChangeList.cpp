#include "VRChangeList.h"

#include <OpenSG/OSGChangeList.h>

using namespace OSG;


VRChangeList::VRChangeList() {}
VRChangeList::~VRChangeList() {}

int VRChangeList::getDestroyed() { return 0; }
int VRChangeList::getCreated() { return 0; }
int VRChangeList::getChanged() { return 0; }

size_t VRChangeList::getTotalEntities() { return totalEntites; }

void VRChangeList::update() {
    ;
}
