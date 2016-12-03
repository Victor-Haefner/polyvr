#include "VRPyPolygon.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::polygon>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Polygon",             /*tp_name*/
    sizeof(VRPyPolygon),             /*tp_basicsize*/
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
    "Polygon binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyPolygon::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_ptr,                 /* tp_new */
};

PyMethodDef VRPyPolygon::methods[] = {
    {"addPoint", (PyCFunction)VRPyPolygon::addPoint, METH_VARARGS, "Get the position - [x,y,z] addPoint()" },
    {"getPoint", (PyCFunction)VRPyPolygon::getPoint, METH_VARARGS, "Get the direction - [x,y,z] getPoint()" },
    {"getPoints", (PyCFunction)VRPyPolygon::getPoints, METH_NOARGS, "Get the up vector - [x,y,z] getPoints()" },
    {"getPoints", (PyCFunction)VRPyPolygon::getPoints, METH_NOARGS, "Get the up vector - [x,y,z] getPoints()" },
    {"close", (PyCFunction)VRPyPolygon::close, METH_NOARGS, "Set the pose - close([pos], [dir], [up])" },
    {"size", (PyCFunction)VRPyPolygon::size, METH_NOARGS, "Transform a vector - size([vec])" },
    {"set", (PyCFunction)VRPyPolygon::set, METH_VARARGS, "Transform back a vector - set([vec])" },
    {"clear", (PyCFunction)VRPyPolygon::clear, METH_NOARGS, "Transform back a vector - clear([vec])" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPolygon::addPoint(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "O", &v)) return NULL;
    self->objPtr->addPoint( parseVec2fList(v) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::getPoint(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return toPyTuple( self->objPtr->getPoint(i) );
}

PyObject* VRPyPolygon::getPoints(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    auto vec = self->objPtr->get();
    PyObject* res = PyList_New(vec.size());
    for (uint i=0; i<vec.size(); i++) PyList_SetItem(res, i, toPyTuple(vec[i]));
    return res;
}

PyObject* VRPyPolygon::close(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    self->objPtr->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::size(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->size() );
}

PyObject* VRPyPolygon::set(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "O", &v)) return NULL;
    vector<OSG::Vec2f> vec;
    for (int i=0; i<pySize(v); i++) vec.push_back( parseVec2fList( PyList_GetItem(v,i) ) );
    self->objPtr->set(vec);
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::clear(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}


