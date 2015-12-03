#ifndef VRPYMILLINGWORKPIECE_H_INCLUDED
#define VRPYMILLINGWORKPIECE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMillingWorkPiece.h"

struct VRPyMillingWorkPiece : VRPyBaseT<OSG::VRMillingWorkPiece> {
    static PyMethodDef methods[];

    static PyObject* reset(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* setCuttingTool(VRPyMillingWorkPiece* self, PyObject* args);
};

#endif // VRPYMILLINGWORKPIECE_H_INCLUDED
