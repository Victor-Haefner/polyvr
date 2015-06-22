#include "VRPySimViDekont.h"
#include "SimViDekont.h"
#include "Player.h"
#include "../../core/scripting/VRPyObject.h"
#include "../../core/scripting/VRPyBaseT.h"

OSG::SimViDekont* VRPySimViDekont::svd = 0;

template<> PyTypeObject VRPyBaseT<OSG::SimViDekont>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.SimViDekont",             /*tp_name*/
    sizeof(VRPySimViDekont),             /*tp_basicsize*/
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
    "SimViDekont bindings",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySimViDekont::methods,             /* tp_methods */
    VRPySimViDekont::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMemberDef VRPySimViDekont::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySimViDekont::methods[] = {
    {"load", (PyCFunction)VRPySimViDekont::load, METH_VARARGS, "set CSG operation - setOperation(string s)" },
    {"update", (PyCFunction)VRPySimViDekont::update, METH_NOARGS, "set CSG operation - setOperation(string s)" },
    {"play", (PyCFunction)VRPySimViDekont::play, METH_NOARGS, "set CSG object edit mode - setEditMode(bool b)" },
    {"stop", (PyCFunction)VRPySimViDekont::stop, METH_NOARGS, "set CSG object edit mode - setEditMode(bool b)" },
    {"rewind", (PyCFunction)VRPySimViDekont::rewind, METH_NOARGS, "set CSG object edit mode - setEditMode(bool b)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySimViDekont::load(VRPySimViDekont* self, PyObject* args) {
    VRPyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    if (child->obj == 0) { PyErr_SetString(err, "VRPySimViDekont::load, root is invalid"); return NULL; }

    if (self->svd == 0) {
        self->svd = new OSG::SimViDekont();
        self->svd->getAnchor()->setPersistency(0);
    }
    self->svd->getAnchor()->switchParent(child->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPySimViDekont::play(VRPySimViDekont* self) {
    if (self->svd == 0) { PyErr_SetString(err, "VRPySimViDekont::play, obj is invalid"); return NULL; }
    self->svd->getPlayer()->play();
    Py_RETURN_TRUE;
}

PyObject* VRPySimViDekont::update(VRPySimViDekont* self) {
    if (self->svd == 0) { PyErr_SetString(err, "VRPySimViDekont::play, obj is invalid"); return NULL; }
    self->svd->getPlayer()->update();
    Py_RETURN_TRUE;
}

PyObject* VRPySimViDekont::stop(VRPySimViDekont* self) {
    if (self->svd == 0) { PyErr_SetString(err, "VRPySimViDekont::play, obj is invalid"); return NULL; }
    self->svd->getPlayer()->stop();
    Py_RETURN_TRUE;
}

PyObject* VRPySimViDekont::rewind(VRPySimViDekont* self) {
    if (self->svd == 0) { PyErr_SetString(err, "VRPySimViDekont::play, obj is invalid"); return NULL; }
    self->svd->getPlayer()->play(true);
    Py_RETURN_TRUE;
}
