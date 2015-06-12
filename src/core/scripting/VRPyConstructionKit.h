#ifndef VRPYCONSTRUCTIONKIT_H_INCLUDED
#define VRPYCONSTRUCTIONKIT_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRConstructionKit.h"

struct VRPyConstructionKit : VRPyBaseT<OSG::VRConstructionKit> {
    static PyMethodDef methods[];

    static PyObject* getSnappingEngine(VRPyConstructionKit* self);
    static PyObject* getSelector(VRPyConstructionKit* self);
    static PyObject* addAnchorType(VRPyConstructionKit* self, PyObject* args);
    static PyObject* addObjectAnchor(VRPyConstructionKit* self, PyObject* args);
    static PyObject* addObject(VRPyConstructionKit* self, PyObject* args);
    static PyObject* breakup(VRPyConstructionKit* self, PyObject* args);
};

#endif // VRPYCONSTRUCTIONKIT_H_INCLUDED
