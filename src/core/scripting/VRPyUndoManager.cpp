#include "VRPyUndoManager.h"
#include "VRPyObject.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(UndoManager, New_ptr);

PyMethodDef VRPyUndoManager::methods[] = {
    {"addObject", (PyCFunction)VRPyUndoManager::addObject, METH_VARARGS, "Add an object - addObject( obj )" },
    {"undo", (PyCFunction)VRPyUndoManager::undo, METH_NOARGS, "Undo last action - undo()" },
    {"redo", (PyCFunction)VRPyUndoManager::redo, METH_NOARGS, "Redo last undo - redo()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyUndoManager::addObject(VRPyUndoManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    if (isNone((PyObject*)o)) { PyErr_SetString(err, "VRPyMolecule::substitute - molecule is invalid"); return NULL; }
    self->objPtr->addObject(o->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyUndoManager::undo(VRPyUndoManager* self) {
    if (!self->valid()) return NULL;
    self->objPtr->undo();
    Py_RETURN_TRUE;
}

PyObject* VRPyUndoManager::redo(VRPyUndoManager* self) {
    if (!self->valid()) return NULL;
    self->objPtr->redo();
    Py_RETURN_TRUE;
}


