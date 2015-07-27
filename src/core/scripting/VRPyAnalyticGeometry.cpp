#include "VRPyAnalyticGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"


template<> PyTypeObject VRPyBaseT<OSG::VRAnalyticGeometry>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.AnalyticGeometry",             /*tp_name*/
    sizeof(VRPyAnalyticGeometry),             /*tp_basicsize*/
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
    "AnalyticGeometry binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyAnalyticGeometry::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_unnamed,                 /* tp_new */
};

PyMethodDef VRPyAnalyticGeometry::methods[] = {
    {"setVector", (PyCFunction)VRPyAnalyticGeometry::setVector, METH_VARARGS, "Add/set a an annotated vector - setVector(int i, [pos], [vec], [col], str label)" },
    {"setLabelSize", (PyCFunction)VRPyAnalyticGeometry::setLabelSize, METH_VARARGS, "Set the size of the labels - setLabelSize( float s )" },
    {"clear", (PyCFunction)VRPyAnalyticGeometry::clear, METH_NOARGS, "Clear data" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnalyticGeometry::setLabelSize(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::setLabelSize - Object is invalid"); return NULL; }
    self->obj->setLabelSize( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyAnalyticGeometry::setVector(VRPyAnalyticGeometry* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::set - Object is invalid"); return NULL; }

    int i;
    PyObject *p, *v, *c, *s;
    if (! PyArg_ParseTuple(args, "iOOOO", &i, &p, &v, &c, &s)) return NULL;

    self->obj->setVector(i, parseVec3fList(p), parseVec3fList(v), parseVec3fList(c), PyString_AsString(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnalyticGeometry::clear(VRPyAnalyticGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnalyticGeometry::clear - Object is invalid"); return NULL; }
    self->obj->clear();
    Py_RETURN_TRUE;
}
