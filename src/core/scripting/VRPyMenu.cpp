#include "VRPyMenu.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRMenu>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Menu",             /*tp_name*/
    sizeof(VRPyMenu),             /*tp_basicsize*/
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
    "Menu binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMenu::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_optional,                 /* tp_new */
};

PyMethodDef VRPyMenu::methods[] = {
    {"append", (PyCFunction)VRPyMenu::append, METH_VARARGS, "Append a child menu - append(str texture_path)" },
    {"setLeafType", (PyCFunction)VRPyMenu::setLeafType, METH_VARARGS, "Set menu layout - setLeafType(str type, float scale)" },
    {"setLayout", (PyCFunction)VRPyMenu::setLayout, METH_VARARGS, "Set menu layout - setLayout(str layout, float param)" },
    {"open", (PyCFunction)VRPyMenu::open, METH_NOARGS, "Open menu" },
    {"close", (PyCFunction)VRPyMenu::close, METH_NOARGS, "Close menu" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMenu::open(VRPyMenu* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::open - Object is invalid"); return NULL; }
    self->obj->open();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::close(VRPyMenu* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::close - Object is invalid"); return NULL; }
    self->obj->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::setLeafType(VRPyMenu* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    PyObject *t, *s;
    if (!PyArg_ParseTuple(args, "OO", &t, &s)) return NULL;
    string ts = PyString_AsString(t);
    OSG::VRMenu::TYPE type;
    if (ts == "SPRITE") type = OSG::VRMenu::SPRITE;
    self->obj->setLeafType( type, parseVec2fList(s));
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::setLayout(VRPyMenu* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    PyObject* l; float p;
    if (!PyArg_ParseTuple(args, "Of", &l, &p)) return NULL;
    string ls = PyString_AsString(l);
    OSG::VRMenu::LAYOUT layout;
    if (ls == "LINEAR") layout = OSG::VRMenu::LINEAR;
    if (ls == "CIRCULAR") layout = OSG::VRMenu::CIRCULAR;
    self->obj->setLayout( layout, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::append(VRPyMenu* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::append - Object is invalid"); return NULL; }
    self->obj->append( parseString(args) );
    Py_RETURN_TRUE;
}
