#include "VRPyColorChooser.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ColorChooser, New_ptr);

PyMethodDef VRPyColorChooser::methods[] = {
    {"setGeometry", (PyCFunction)VRPyColorChooser::setGeometry, METH_VARARGS, "Set the geometry" },
    {"set", (PyCFunction)VRPyColorChooser::setColor, METH_VARARGS, "Set the color - set(r,g,b)" },
    {"get", (PyCFunction)VRPyColorChooser::getColor, METH_NOARGS, "Return the active color" },
    {"getLast", (PyCFunction)VRPyColorChooser::getLastColor, METH_NOARGS, "Return the previous color" },
    {"resolve", (PyCFunction)VRPyColorChooser::resolve, METH_VARARGS, "Get the color from device interaction - resolve(dev)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyColorChooser::setGeometry(VRPyColorChooser* self, PyObject* args) {
	if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    parseObject(args, geo);
    if (geo) self->objPtr->setGeometry(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::resolve(VRPyColorChooser* self, PyObject* args) {
	if (!self->valid()) return NULL;
    VRPyDevice* dev = 0;
    parseObject(args, dev);
    if (dev) self->objPtr->resolve(dev->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::getColor(VRPyColorChooser* self) {
	if (!self->valid()) return NULL;
    return toPyTuple( Vec3d(self->objPtr->getColor()) );
}

PyObject* VRPyColorChooser::getLastColor(VRPyColorChooser* self) {
	if (!self->valid()) return NULL;
    return toPyTuple( Vec3d(self->objPtr->getLastColor()) );
}

PyObject* VRPyColorChooser::setColor(VRPyColorChooser* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setColor( Vec3f(parseVec3d(args)) );
    Py_RETURN_TRUE;
}
