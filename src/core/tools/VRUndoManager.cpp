#include "VRUndoManager.h"
#include "core/utils/VRManager.cpp"
#include "core/objects/object/VRObject.h"

using namespace OSG;


VRUndoAtom::VRUndoAtom(string name) {
    setNameSpace("UndoAtom");
    setName(name);
}

VRUndoAtomPtr VRUndoAtom::create(string name) { return VRUndoAtomPtr( new VRUndoAtom(name) ); }

void VRUndoAtom::set(VRUpdatePtr f_undo, VRUpdatePtr f_redo) { this->f_undo = f_undo; this->f_redo = f_redo; }

void VRUndoAtom::undo() { if (f_undo) (*f_undo)(0); }
void VRUndoAtom::redo() { if (f_redo) (*f_redo)(0); }




VRUndoManager::VRUndoManager() : VRManager<VRUndoAtom>("UndoManager") {}

VRUndoManagerPtr VRUndoManager::create() { return VRUndoManagerPtr( new VRUndoManager() ); }
VRUndoManagerPtr VRUndoManager::ptr() { return static_pointer_cast<VRUndoManager>( shared_from_this() ); }

void VRUndoManager::addObject(VRObjectPtr o) { o->setUndoManager(ptr()); }

void VRUndoManager::recUndo(VRUpdatePtr f_undo, VRUpdatePtr f_redo) {
    if (ward) return;
    /*if (current) {
        // TODO: delete history from here!
    }*/

    int now = VRGlobals::get()->CURRENT_FRAME;
    auto a = VRManager<VRUndoAtom>::add("", now);
    a->set(f_undo, f_redo);
    current = data.rbegin();
}

void VRUndoManager::redo() {
    if (current == data.rbegin()) return;
    ward = true;
    current--;
    current->second->redo();
    ward = false;
}

void VRUndoManager::undo() {
    if (current == data.rend()) return;
    ward = true;
    current->second->undo();
    current++;
    ward = false;
}
