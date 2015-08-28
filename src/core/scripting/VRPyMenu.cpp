#include "VRPyMenu.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#include <boost/bind.hpp>

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
    {"setLeafType", (PyCFunction)VRPyMenu::setLeafType, METH_VARARGS, "Set menu layout - setLeafType(str type, vec2f scale)\n\ttype : ['SPRITE'], scale is the size of the sprite" },
    {"setLayout", (PyCFunction)VRPyMenu::setLayout, METH_VARARGS, "Set menu layout - setLayout(str layout, float param)\n\tlayout : ['LINEAR', 'CIRCULAR'], param is the distance between leafs" },
    {"open", (PyCFunction)VRPyMenu::open, METH_NOARGS, "Open menu" },
    {"close", (PyCFunction)VRPyMenu::close, METH_NOARGS, "Close menu" },
    {"setCallback", (PyCFunction)VRPyMenu::setCallback, METH_VARARGS, "Set a menu callback - setCallback(fkt, [params])" },
    {"trigger", (PyCFunction)VRPyMenu::trigger, METH_NOARGS, "Trigger menu or enter next layer if no callback is set" },
    {"move", (PyCFunction)VRPyMenu::setCallback, METH_VARARGS, "Move the cursor - move(int dir)\n\tleft: dir=-1, right: dir=1" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMenu::trigger(VRPyMenu* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::trigger - Object is invalid"); return NULL; }
    self->obj->trigger();
    Py_RETURN_TRUE;
}

PyObject* VRPyMenu::move(VRPyMenu* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMenu::move - Object is invalid"); return NULL; }
    self->obj->move( parseInt(args) );
    Py_RETURN_TRUE;
}

void execCall(PyObject* pyFkt, PyObject* pArgs, OSG::VRMenu* menu) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    if (pArgs == 0) pArgs = PyTuple_New(0);
    PyObject_CallObject(pyFkt, pArgs);

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}

PyObject* VRPyMenu::setCallback(VRPyMenu* self, PyObject *args) {
    PyObject *pyFkt, *pArgs = 0;
    if (PyTuple_Size(args) == 1) { if (! PyArg_ParseTuple(args, "O", &pyFkt)) return NULL; }
    else if (! PyArg_ParseTuple(args, "OO", &pyFkt, &pArgs)) return NULL;
    Py_IncRef(pyFkt);

    if (pArgs != 0) {
        std::string type = pArgs->ob_type->tp_name;
        if (type == "list") pArgs = PyList_AsTuple(pArgs);
    }

    Py_IncRef(pArgs);
    self->obj->setCallback(new VRFunction<OSG::VRMenu*>( "pyMenuCB", boost::bind(execCall, pyFkt, pArgs, _1) ));
    Py_RETURN_TRUE;
}

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
    return fromPtr( self->obj->append( parseString(args) ) );
}
