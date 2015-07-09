#include "VRPyCSG.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

#include <OpenSG/OSGGeoProperties.h>

template<> PyTypeObject VRPyBaseT<OSG::CSGGeometry>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.CSGGeometry",             /*tp_name*/
    sizeof(VRPyCSG),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRCSG binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyCSG::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMethodDef VRPyCSG::methods[] = {
    {"getOperation", (PyCFunction)VRPyCSG::getOperation, METH_VARARGS, "get CSG operation" },
    {"setOperation", (PyCFunction)VRPyCSG::setOperation, METH_VARARGS, "set CSG operation - setOperation(string s)\n use one of: 'unite', 'subtract', 'intersect'" },
    {"getEditMode", (PyCFunction)VRPyCSG::getEditMode, METH_VARARGS, "get CSG object edit mode" },
    {"setEditMode", (PyCFunction)VRPyCSG::setEditMode, METH_VARARGS, "set CSG object edit mode, set it to false to compute and show the result - setEditMode(bool b)" },
    {"markEdges", (PyCFunction)VRPyCSG::markEdges, METH_VARARGS, "Color the edges of the polyhedron, pass a list of int pairs - markEdges([[i1,i2],[i1,i3],...])\nPass an empty list to hide edges." },
    {"setThreshold", (PyCFunction)VRPyCSG::setThreshold, METH_VARARGS, "Set the threashold used to merge double vertices - setThreshold( float )\n default is 1e-4" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCSG::setThreshold(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::setThreshold, Object is invalid"); return NULL; }
    auto t = parseVec2f(args);
    self->obj->setThreshold( t[0], t[1] );
    Py_RETURN_TRUE;
}

PyObject* VRPyCSG::markEdges(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::markEdges, Object is invalid"); return NULL; }

    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (pySize(vec) == 0) Py_RETURN_TRUE;

    vector<OSG::Vec2i> edges;
    pyListToVector<vector<OSG::Vec2i>, OSG::Vec2i>(vec, edges);
    self->obj->markEdges(edges);
    Py_RETURN_TRUE;
}

PyObject* VRPyCSG::getOperation(VRPyCSG* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::getOperation, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getOperation().c_str());
}

PyObject* VRPyCSG::setOperation(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::setOperation, Object is invalid"); return NULL; }
    self->obj->setOperation( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyCSG::getEditMode(VRPyCSG* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::getOperation, Object is invalid"); return NULL; }
    return PyBool_FromLong(self->obj->getEditMode());
}

PyObject* VRPyCSG::setEditMode(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::setEditMode, Object is invalid"); return NULL; }
    bool b = parseBool(args);
	return PyBool_FromLong(self->obj->setEditMode(b));
}
