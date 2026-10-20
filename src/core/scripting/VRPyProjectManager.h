#ifndef VRPYPROJECTMANAGER_H_INCLUDED
#define VRPYPROJECTMANAGER_H_INCLUDED

#include "core/tools/VRProjectManager.h"
#include "VRPyBase.h"

struct VRPyStorageContext : public VRPyBaseT<OSG::VRStorageContext> {
    static PyMethodDef methods[];
};

struct VRPyStorage : public VRPyBaseT<OSG::VRStorage> {
    static PyMethodDef methods[];

    static PyObject* fromSharedPtr(OSG::VRStoragePtr obj);
};

struct VRPyProjectManager : public VRPyBaseT<OSG::VRProjectManager> {
    static PyMethodDef methods[];
};

#endif // VRPYPROJECTMANAGER_H_INCLUDED
