#include "VRPyWorldGenerator.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(Asphalt, New_ptr);
simpleVRPyType(RoadNetwork, New_ptr);


PyMethodDef VRPyAsphalt::methods[] = {
    {"addMarking", (PyCFunction)VRPyAsphalt::addMarking, METH_VARARGS, "Add marking - addMarking( rID, path, width, dashN )" },
    {"addTrack", (PyCFunction)VRPyAsphalt::addTrack, METH_VARARGS, "Add track - addTrack( rID, path, width, dashN )" },
    {"clearTexture", (PyCFunction)VRPyAsphalt::clearTexture, METH_NOARGS, "Clear internal textures - clearTexture()" },
    {"updateTexture", (PyCFunction)VRPyAsphalt::updateTexture, METH_NOARGS, "Update internal textures - updateTexture()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAsphalt::addMarking(VRPyAsphalt* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int rID = 0;
    int dashN = 0;
    float width = 0.1;
    VRPyPath* path = 0;
    if (!PyArg_ParseTuple(args, "iOf|i", &rID, &path, &width, &dashN)) return NULL;
    self->objPtr->addMarking( rID, path->objPtr, width, dashN );
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::addTrack(VRPyAsphalt* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int rID = 0;
    int dashN = 0;
    float width = 0.1;
    VRPyPath* path = 0;
    if (!PyArg_ParseTuple(args, "iOf|i", &rID, &path, &width, &dashN)) return NULL;
    self->objPtr->addTrack( rID, path->objPtr, width, dashN );
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::clearTexture(VRPyAsphalt* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clearTexture();
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::updateTexture(VRPyAsphalt* self) {
    if (!self->valid()) return NULL;
    self->objPtr->updateTexture();
    Py_RETURN_TRUE;
}


// ------------------------------------------------------

PyMethodDef VRPyRoadNetwork::methods[] = {
    {NULL}  /* Sentinel */
};
