#include "VRPyAnnotationEngine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"


template<> PyTypeObject VRPyBaseT<OSG::VRAnnotationEngine>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.AnnotationEngine",             /*tp_name*/
    sizeof(VRPyAnnotationEngine),             /*tp_basicsize*/
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
    "AnnotationEngine binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyAnnotationEngine::methods,             /* tp_methods */
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

PyMethodDef VRPyAnnotationEngine::methods[] = {
    {"set", (PyCFunction)VRPyAnnotationEngine::set, METH_VARARGS, "Set label - set(int i, [x,y,z] pos, str val)" },
    {"clear", (PyCFunction)VRPyAnnotationEngine::clear, METH_NOARGS, "Clear numbers" },
    {"setSize", (PyCFunction)VRPyAnnotationEngine::setSize, METH_VARARGS, "Set font height - setSize( float )" },
    {"setColor", (PyCFunction)VRPyAnnotationEngine::setColor, METH_VARARGS, "Set font color - setColor( [r,g,b,a] )" },
    {"setBackground", (PyCFunction)VRPyAnnotationEngine::setBackground, METH_VARARGS, "Set background color - setBackground( [r,g,b,a] )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnnotationEngine::setSize(VRPyAnnotationEngine* self, PyObject* args) {
    self->obj->setSize(parseFloat(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::setColor(VRPyAnnotationEngine* self, PyObject* args) {
    self->obj->setColor(parseVec4f(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::setBackground(VRPyAnnotationEngine* self, PyObject* args) {
    self->obj->setBackground(parseVec4f(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::set(VRPyAnnotationEngine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnnotationEngine::set - Object is invalid"); return NULL; }

    int i;
    PyObject* s;
    PyObject* p;
    if (! PyArg_ParseTuple(args, "iOO", &i, &p, &s)) return NULL;

    self->obj->set(i, parseVec3fList(p), PyString_AsString(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyAnnotationEngine::clear(VRPyAnnotationEngine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyAnnotationEngine::clear - Object is invalid"); return NULL; }
    self->obj->clear();
    Py_RETURN_TRUE;
}
