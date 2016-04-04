#include "VRUndoInterface.h"

using namespace OSG;

VRUndoInterface::VRUndoInterface() {}

void VRUndoInterface::setUndoManager(VRUndoManagerPtr mgr) { undo = mgr; }
