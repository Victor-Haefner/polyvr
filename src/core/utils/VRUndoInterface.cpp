#include "VRUndoInterface.h"
#include "core/tools/VRUndoManager.h"

using namespace OSG;

VRUndoInterface::VRUndoInterface() {}
VRUndoInterface::~VRUndoInterface() {}

void VRUndoInterface::setUndoManager(VRUndoManagerPtr mgr) { undo = mgr; initiated = true; }

bool VRUndoInterface::undoInitiated() { return initiated; }
bool VRUndoInterface::isUndoing() {
    auto u = undo.lock();
    return u ? u->isUndoing() : false;
}
