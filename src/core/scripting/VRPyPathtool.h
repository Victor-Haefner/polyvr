#ifndef VRPYPATHTOOL_H_INCLUDED
#define VRPYPATHTOOL_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRPathtool.h"

struct VRPyPathtool : VRPyBaseT<OSG::VRPathtool> {
    static PyMethodDef methods[];

    static PyObject* newPath(VRPyPathtool* self, PyObject* args);
    static PyObject* addPath(VRPyPathtool* self, PyObject* args);
    static PyObject* remPath(VRPyPathtool* self, PyObject* args);
    static PyObject* extrude(VRPyPathtool* self, PyObject* args);
    static PyObject* select(VRPyPathtool* self, PyObject* args);
    static PyObject* setVisible(VRPyPathtool* self, PyObject* args);
    static PyObject* getPaths(VRPyPathtool* self);
    static PyObject* update(VRPyPathtool* self);
    static PyObject* getHandles(VRPyPathtool* self, PyObject* args);
    static PyObject* getStroke(VRPyPathtool* self, PyObject* args);
    static PyObject* clear(VRPyPathtool* self, PyObject* args);
};

#endif // VRPYPATHTOOL_H_INCLUDED
