#include "VRUndoManager.h"
#include "core/utils/VRManager.cpp"
#include "core/objects/object/VRObject.h"

using namespace OSG;


VRUndoAtom::VRUndoAtom(string name) {
    setNameSpace("UndoAtom");
    setName(name);
}

VRUndoAtomPtr VRUndoAtom::create(string name) { return VRUndoAtomPtr( new VRUndoAtom(name) ); }

void VRUndoAtom::set(VRUpdateCbPtr f_undo, VRUpdateCbPtr f_redo, VREvalCbPtr f_valid) { this->f_undo = f_undo; this->f_redo = f_redo; this->f_valid = f_valid; }

bool VRUndoAtom::valid() { bool b; (*f_valid)(b); return b; }
bool VRUndoAtom::undo() { if (f_undo && valid()) { (*f_undo)(); } else return 0; return 1;}
bool VRUndoAtom::redo() { if (f_redo && valid()) { (*f_redo)(); } else return 0; return 1; }




VRUndoManager::VRUndoManager() : VRManager<VRUndoAtom>("UndoManager") {}

VRUndoManagerPtr VRUndoManager::create() { return VRUndoManagerPtr( new VRUndoManager() ); }
VRUndoManagerPtr VRUndoManager::ptr() { return static_pointer_cast<VRUndoManager>( shared_from_this() ); }

void VRUndoManager::addObject(VRObjectPtr o) { o->setUndoManager(ptr()); }
bool VRUndoManager::isUndoing() { return undoing; }

void VRUndoManager::recUndo(VRUpdateCbPtr f_undo, VRUpdateCbPtr f_redo, VREvalCbPtr f_valid) {
    if (ward) return;
    data.erase(current.base(), data.end()); // delete history from here
    auto a = VRManager<VRUndoAtom>::add("", key);
    a->set(f_undo, f_redo, f_valid);
    current = data.rbegin();
    key++;
}

void VRUndoManager::redo() {
    if (current == data.rbegin()) return;
    undoing = true;
    ward = true;
    current--;
    bool r = current->second->redo();
    ward = false;
    if (!r) redo();
    undoing = false;
}

void VRUndoManager::undo() {
    if (current == data.rend()) return;
    undoing = true;
    ward = true;
    bool u = current->second->undo();
    current++;
    ward = false;
    if (!u) undo();
    undoing = false;
}
