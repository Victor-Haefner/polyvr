#include "VRUndoInterface.h"

using namespace OSG;

VRUndoInterface::VRUndoInterface() {}
VRUndoInterface::~VRUndoInterface() {}

void VRUndoInterface::setUndoManager(VRUndoManagerPtr mgr) { undo = mgr; initiated = true; }

bool VRUndoInterface::undoInitiated() { return initiated; }
