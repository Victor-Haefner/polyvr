#ifndef VRPYSPATIALCOLLISIONMANAGER_H_INCLUDED
#define VRPYSPATIALCOLLISIONMANAGER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/objects/geometry/VRSpatialCollisionManager.h"

struct VRPySpatialCollisionManager : VRPyBaseT<OSG::VRSpatialCollisionManager> {
    static PyMethodDef methods[];
    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

#endif // VRPYSPATIALCOLLISIONMANAGER_H_INCLUDED
