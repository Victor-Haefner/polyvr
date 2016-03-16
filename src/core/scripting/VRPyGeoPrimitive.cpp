#include "VRPyGeoPrimitive.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(GeoPrimitive, New_VRObjects_ptr);

PyMethodDef VRPyGeoPrimitive::methods[] = {
    {"select", (PyCFunction)VRPyGeoPrimitive::select, METH_VARARGS, "Activate or deactivate the editing handles - select( bool )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyGeoPrimitive::select(VRPyGeoPrimitive* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->select( parseBool(args) );
    Py_RETURN_TRUE;
}
