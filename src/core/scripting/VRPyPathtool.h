#ifndef VRPYPATHTOOL_H_INCLUDED
#define VRPYPATHTOOL_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRPathtool.h"

struct VRPyPathtool : VRPyBaseT<OSG::VRPathtool> {
    static PyMethodDef methods[];

    static PyObject* newPath(VRPyPathtool* self, PyObject* args);
    static PyObject* setGraph(VRPyPathtool* self, PyObject* args);
    static PyObject* addPath(VRPyPathtool* self, PyObject* args);
    static PyObject* remPath(VRPyPathtool* self, PyObject* args);
    static PyObject* extrude(VRPyPathtool* self, PyObject* args);
    static PyObject* select(VRPyPathtool* self, PyObject* args);
    static PyObject* deselect(VRPyPathtool* self);
    static PyObject* setVisible(VRPyPathtool* self, PyObject* args);
    static PyObject* getPaths(VRPyPathtool* self);
    static PyObject* getPath(VRPyPathtool* self, PyObject* args);
    static PyObject* update(VRPyPathtool* self);
    static PyObject* getHandles(VRPyPathtool* self, PyObject* args);
    static PyObject* getStroke(VRPyPathtool* self, PyObject* args);
    static PyObject* clear(VRPyPathtool* self, PyObject* args);
    static PyObject* setHandleGeometry(VRPyPathtool* self, PyObject* args);
    static PyObject* getPathMaterial(VRPyPathtool* self);
    static PyObject* addNode(VRPyPathtool* self, PyObject* args);
    static PyObject* removeNode(VRPyPathtool* self, PyObject* args);
    static PyObject* getNodeID(VRPyPathtool* self, PyObject* args);
    static PyObject* connect(VRPyPathtool* self, PyObject* args);
    static PyObject* disconnect(VRPyPathtool* self, PyObject* args);
};

#endif // VRPYPATHTOOL_H_INCLUDED
