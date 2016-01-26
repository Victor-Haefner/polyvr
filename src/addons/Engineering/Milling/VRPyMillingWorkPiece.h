#ifndef VRPYMILLINGWORKPIECE_H_INCLUDED
#define VRPYMILLINGWORKPIECE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMillingWorkPiece.h"

struct VRPyMillingWorkPiece : VRPyBaseT<OSG::VRMillingWorkPiece> {
    static PyMethodDef methods[];

    static PyObject* init(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* reset(VRPyMillingWorkPiece* self);
    static PyObject* setCuttingTool(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* setCuttingToolProfile(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* setRefreshWait(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* setLevelsPerGeometry(VRPyMillingWorkPiece* self, PyObject* args);
    static PyObject* updateGeometry(VRPyMillingWorkPiece* self, PyObject* args);
};

#endif // VRPYMILLINGWORKPIECE_H_INCLUDED
