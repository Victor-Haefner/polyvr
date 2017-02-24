#ifndef VRPYOBJECTMANAGER_H_INCLUDED
#define VRPYOBJECTMANAGER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/scene/VRObjectManager.h"

struct VRPyObjectManager : VRPyBaseT<OSG::VRObjectManager> {
    static PyMethodDef methods[];

    static PyObject* add(VRPyObjectManager* self, PyObject* args);
    static PyObject* copy(VRPyObjectManager* self, PyObject* args);
    static PyObject* clear(VRPyObjectManager* self);
    static PyObject* get(VRPyObjectManager* self, PyObject* args);
    static PyObject* remove(VRPyObjectManager* self, PyObject* args);
    static PyObject* addTemplate(VRPyObjectManager* self, PyObject* args);
    static PyObject* getTemplate(VRPyObjectManager* self, PyObject* args);
    static PyObject* getCatalog(VRPyObjectManager* self);
};

#endif // VRPYOBJECTMANAGER_H_INCLUDED
