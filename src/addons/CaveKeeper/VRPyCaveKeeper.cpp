#include "VRPyCaveKeeper.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyDevice.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simplePyType(CaveKeeper, New_ptr);

PyMethodDef VRPyCaveKeeper::methods[] = {
    {"init", (PyCFunction)VRPyCaveKeeper::initWorld, METH_VARARGS, "Init real world" },
    {"intersect", (PyCFunction)VRPyCaveKeeper::intersect, METH_VARARGS, "Intersect a cube - int intersect(dev)" },
    {"remBlock", (PyCFunction)VRPyCaveKeeper::remBlock, METH_VARARGS, "Remove a cube - remBlock(i)" },
    {"addBlock", (PyCFunction)VRPyCaveKeeper::addBlock, METH_VARARGS, "Add a cube - addBlock([x,y,z])" },
    {"addObject", (PyCFunction)VRPyCaveKeeper::addObject, METH_VARARGS, "Place an object - addObject([x,y,z])" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCaveKeeper::initWorld(VRPyCaveKeeper* self, PyObject* args) {
    VRPyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    child->objPtr->addChild(self->objPtr->getAnchor());
    Py_RETURN_TRUE;
}

PyObject* VRPyCaveKeeper::intersect(VRPyCaveKeeper* self, PyObject* args) {
    VRPyDevice* dev = NULL;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    return PyInt_FromLong( self->objPtr->intersect(dev->objPtr) );
}

PyObject* VRPyCaveKeeper::addBlock(VRPyCaveKeeper* self, PyObject* args) {
    return PyInt_FromLong( self->objPtr->addBlock( parseVec3i(args)) );
}

PyObject* VRPyCaveKeeper::remBlock(VRPyCaveKeeper* self, PyObject* args) {
    self->objPtr->remBlock( parseInt(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyCaveKeeper::addObject(VRPyCaveKeeper* self, PyObject* args) {
    VRPyDevice* dev = NULL;
    const char *obj_t;
    VRPyTransform* geo = NULL;
    if (! PyArg_ParseTuple(args, "OsO", &dev, &obj_t, &geo)) return NULL;
    self->objPtr->place(dev->objPtr, obj_t, geo->objPtr);
    Py_RETURN_TRUE;
}
