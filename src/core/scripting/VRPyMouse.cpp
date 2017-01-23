#include "VRPyMouse.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRMouse>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Mouse",             /*tp_name*/
    sizeof(VRPyMouse),             /*tp_basicsize*/
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
    "Mouse binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMouse::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef VRPyMouse::methods[] = {
    {"setCursor", (PyCFunction)VRPyMouse::setCursor, METH_VARARGS, "Set the mouse cursor - setCursor(str cursor)\n\tcursor can be: 'WATCH', 'PLUS', ...  \n\tsee here for more: http://www.pygtk.org/pygtk2reference/class-gdkcursor.html" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyMouse::setCursor(VRPyMouse* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* c = 0;
    if (! PyArg_ParseTuple(args, "s:setCursor", &c)) return NULL;
    self->objPtr->setCursor(c);
    Py_RETURN_TRUE;
}




