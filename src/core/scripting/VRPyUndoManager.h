#ifndef VRPYUNDOMANAGER_H_INCLUDED
#define VRPYUNDOMANAGER_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRUndoManager.h"

struct VRPyUndoManager : VRPyBaseT<OSG::VRUndoManager> {
    static PyMethodDef methods[];

    static PyObject* addObject(VRPyUndoManager* self, PyObject* args);
    static PyObject* undo(VRPyUndoManager* self);
    static PyObject* redo(VRPyUndoManager* self);
};

#endif // VRPYUNDOMANAGER_H_INCLUDED
