#include "VRPyColorChooser.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ColorChooser, New);

PyMethodDef VRPyColorChooser::methods[] = {
    {"setGeometry", (PyCFunction)VRPyColorChooser::setGeometry, METH_VARARGS, "Set the geometry" },
    {"set", (PyCFunction)VRPyColorChooser::setColor, METH_VARARGS, "Set the color - set(r,g,b)" },
    {"get", (PyCFunction)VRPyColorChooser::getColor, METH_NOARGS, "Return the active color" },
    {"getLast", (PyCFunction)VRPyColorChooser::getLastColor, METH_NOARGS, "Return the previous color" },
    {"resolve", (PyCFunction)VRPyColorChooser::resolve, METH_VARARGS, "Get the color from device interaction - resolve(dev)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyColorChooser::setGeometry(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::activate - Object is invalid"); return NULL; }
    VRPyGeometry* geo = 0;
    parseObject(args, geo);
    if (geo) self->obj->setGeometry(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::resolve(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::activate - Object is invalid"); return NULL; }
    VRPyDevice* dev = 0;
    parseObject(args, dev);
    if (dev) self->obj->resolve(dev->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::getColor(VRPyColorChooser* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getColor() );
}

PyObject* VRPyColorChooser::getLastColor(VRPyColorChooser* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getLastColor() );
}

PyObject* VRPyColorChooser::setColor(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    self->obj->setColor( parseVec3f(args) );
    Py_RETURN_TRUE;
}
