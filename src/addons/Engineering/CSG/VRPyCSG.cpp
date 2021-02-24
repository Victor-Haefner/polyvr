#include "VRPyCSG.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

simplePyType(CSGGeometry, New_VRObjects_ptr);

PyMethodDef VRPyCSGGeometry::methods[] = {
    {"getOperation", (PyCFunction)VRPyCSGGeometry::getOperation, METH_VARARGS, "get CSG operation" },
    {"setOperation", (PyCFunction)VRPyCSGGeometry::setOperation, METH_VARARGS, "set CSG operation - setOperation(string s)\n use one of: 'unite', 'subtract', 'intersect'" },
    {"getEditMode", (PyCFunction)VRPyCSGGeometry::getEditMode, METH_VARARGS, "get CSG object edit mode" },
    {"setEditMode", (PyCFunction)VRPyCSGGeometry::setEditMode, METH_VARARGS, "set CSG object edit mode, set it to false to compute and show the result - setEditMode(bool b)" },
    {"markEdges", (PyCFunction)VRPyCSGGeometry::markEdges, METH_VARARGS, "Color the edges of the polyhedron, pass a list of int pairs - markEdges([[i1,i2],[i1,i3],...])\nPass an empty list to hide edges." },
    {"setThreshold", (PyCFunction)VRPyCSGGeometry::setThreshold, METH_VARARGS, "Set the threashold used to merge double vertices - setThreshold( float )\n default is 1e-4" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCSGGeometry::setThreshold(VRPyCSGGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::setThreshold, Object is invalid"); return NULL; }
    auto t = parseVec2f(args);
    self->objPtr->setThreshold( t[0], t[1] );
    Py_RETURN_TRUE;
}

PyObject* VRPyCSGGeometry::markEdges(VRPyCSGGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::markEdges, Object is invalid"); return NULL; }

    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (pySize(vec) == 0) Py_RETURN_TRUE;

    vector<OSG::Vec2i> edges;
    pyListToVector<vector<OSG::Vec2i>, OSG::Vec2i>(vec, edges);
    self->objPtr->markEdges(edges);
    Py_RETURN_TRUE;
}

PyObject* VRPyCSGGeometry::getOperation(VRPyCSGGeometry* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::getOperation, Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getOperation().c_str());
}

PyObject* VRPyCSGGeometry::setOperation(VRPyCSGGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::setOperation, Object is invalid"); return NULL; }
    self->objPtr->setOperation( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyCSGGeometry::getEditMode(VRPyCSGGeometry* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::getOperation, Object is invalid"); return NULL; }
    return PyBool_FromLong(self->objPtr->getEditMode());
}

PyObject* VRPyCSGGeometry::setEditMode(VRPyCSGGeometry* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyCSGGeometry::setEditMode, Object is invalid"); return NULL; }
    bool b = parseBool(args);
	return PyBool_FromLong(self->objPtr->setEditMode(b));
}
