#include "VRPyMillingWorkPiece.h"
#include "VRPyMillingCuttingToolProfile.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;

simpleVRPyType(MillingWorkPiece, New_VRObjects_ptr);

PyMethodDef VRPyMillingWorkPiece::methods[] = {
    {"init", (PyCFunction)VRPyMillingWorkPiece::init, METH_VARARGS,
        "Set piece volume parameter and init - init( [int] blocksPerDimension, "
        " float sizeOfSmallestBlock ). sizeOfSmallestBlock is the edge length of the smallest cube"
        " while the blocksPerDimension will determine the size of the Workpiece."
        " For example ([1000, 1000, 1000], 0.01) will give a cube of the length x,y,z = 10"},
    {"reset", (PyCFunction)VRPyMillingWorkPiece::reset, METH_NOARGS,
        "resets the whole milling workpiece"},
    {"setCuttingTool", (PyCFunction)VRPyMillingWorkPiece::setCuttingTool, METH_VARARGS,
        "Set cutting tool geometry - setCuttingTool(geo)" },
    {"setRefreshWait", (PyCFunction)VRPyMillingWorkPiece::setRefreshWait, METH_VARARGS,
    "setRefreshWait(int updatesToWaitFor) sets the milling workpiece to wait at least "
    "updatesToWaitFor updates on the worktools positions to update the geometry of the "
    "workpiece" },
    {"setLevelsPerGeometry", (PyCFunction)VRPyMillingWorkPiece::setLevelsPerGeometry, METH_VARARGS,
    "setLevelsPerGeometry(int levels) - levels means each gemeotry element of the workpiece "
    "will contain 2^levels little cubes. This parameter exists to tune the performance of the "
    "workpiece simulation" },
    {"updateGeometry", (PyCFunction)VRPyMillingWorkPiece::updateGeometry, METH_VARARGS,
    "updateGeometry() - updates the geometry of the workpiece immediately." },
    {"setCuttingToolProfile", (PyCFunction)VRPyMillingWorkPiece::setCuttingToolProfile, METH_VARARGS,
    "setCuttingToolProfile() - sets the profile for the cutting tool."},
    {NULL}  /* Sentinel */
};

PyObject* VRPyMillingWorkPiece::reset(VRPyMillingWorkPiece* self) {
    if (!self->valid()) return NULL;
    self->objPtr->reset();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::init(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* vec; float s;
    if (! PyArg_ParseTuple(args, "Of", &vec, &s)) return NULL;
    auto v = parseVec3iList(vec);
    self->objPtr->init(v,s);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setCuttingTool(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* geo;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    self->objPtr->setCuttingTool( geo->objPtr );
    Py_RETURN_TRUE;
}

//Add of Marie
PyObject* VRPyMillingWorkPiece::setCuttingToolProfile(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyMillingCuttingToolProfile* pyProfile;
    if (!PyArg_ParseTuple(args, "O", &pyProfile)) return NULL;
    self->objPtr->setCuttingProfile(pyProfile->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setRefreshWait(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int refreshWait = 0;
    if (! PyArg_ParseTuple(args, "i", &refreshWait)) return NULL;
    self->objPtr->setRefreshWait(refreshWait);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setLevelsPerGeometry(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int levels = 0;
    if (! PyArg_ParseTuple(args, "i", &levels)) return NULL;
    self->objPtr->setLevelsPerGeometry(levels);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::updateGeometry(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->updateGeometry();
    Py_RETURN_TRUE;
}

