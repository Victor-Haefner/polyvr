#include "VRPyGeoPrimitive.h"
#include "VRPyGeometry.h"
#include "core/objects/geometry/VRHandle.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(GeoPrimitive, New_VRObjects_ptr);

PyMethodDef VRPyGeoPrimitive::methods[] = {
    {"select", (PyCFunction)VRPyGeoPrimitive::select, METH_VARARGS, "Activate or deactivate the editing handles - select( bool )" },
    {"getHandles", (PyCFunction)VRPyGeoPrimitive::getHandles, METH_NOARGS, "Return the editing handles - getHandles()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyGeoPrimitive::select(VRPyGeoPrimitive* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->select( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyGeoPrimitive::getHandles(VRPyGeoPrimitive* self) {
    if (!self->valid()) return NULL;
    auto objs = self->objPtr->getHandles();
    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        OSG::VRGeometryPtr g = dynamic_pointer_cast<OSG::VRGeometry>(objs[i]);
        PyList_SetItem(li, i, VRPyGeometry::fromSharedPtr(g));
    }
    return li;
}
