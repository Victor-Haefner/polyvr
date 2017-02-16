#ifndef VRPYPROJECTMANAGER_H_INCLUDED
#define VRPYPROJECTMANAGER_H_INCLUDED

#include "core/tools/VRProjectManager.h"
#include "VRPyBase.h"

struct VRPyStorage : public VRPyBaseT<OSG::VRStorage> {
    static PyMethodDef methods[];
};

struct VRPyProjectManager : public VRPyBaseT<OSG::VRProjectManager> {
    static PyMethodDef methods[];

    static PyObject* addItem(VRPyProjectManager* self, PyObject* args);
    static PyObject* getItems(VRPyProjectManager* self);
    static PyObject* newp(VRPyProjectManager* self, PyObject* args);
    static PyObject* save(VRPyProjectManager* self, PyObject* args);
    static PyObject* load(VRPyProjectManager* self, PyObject* args);
    static PyObject* setPersistencyLevel(VRPyProjectManager* self, PyObject* args);
};

#endif // VRPYPROJECTMANAGER_H_INCLUDED
