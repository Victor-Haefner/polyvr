#ifndef VRPYPATHTOOL_H_INCLUDED
#define VRPYPATHTOOL_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRPathtool.h"

struct VRPyPathtool : VRPyBaseT<OSG::VRPathtool> {
    static PyMethodDef methods[];

    static PyObject* newPath(VRPyPathtool* self, PyObject* args);
    static PyObject* remPath(VRPyPathtool* self, PyObject* args);
    static PyObject* extrude(VRPyPathtool* self, PyObject* args);
};

#endif // VRPYPATHTOOL_H_INCLUDED
