#include "VRPyWebCam.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(WebCam, New_ptr);

PyMethodDef VRPyWebCam::methods[] = {
    {"connect", (PyCFunction)VRPyWebCam::connect, METH_VARARGS, "Connect to a webcam raw stream - connect(str uri, int res, flt ratio)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyWebCam::connect(VRPyWebCam* self, PyObject* args) {
	if (!self->valid()) return NULL;
	PyObject* uri; int res; float ratio;
    if (! PyArg_ParseTuple(args, "Oif", &uri, &res, &ratio)) return NULL;
	self->objPtr->connect( PyString_AsString(uri), res, ratio);
    Py_RETURN_TRUE;
}
