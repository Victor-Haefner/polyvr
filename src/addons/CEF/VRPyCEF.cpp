#include "VRPyCEF.h"
#include "../../core/scripting/VRPyMaterial.h"
#include "../../core/scripting/VRPyObject.h"
#include "../../core/scripting/VRPyDevice.h"
#include "../../core/scripting/VRPyBaseT.h"
#include <OpenSG/OSGMatrix.h>

template<> PyTypeObject VRPyBaseT<CEF>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.CEF",             /*tp_name*/
    sizeof(VRPyCEF),             /*tp_basicsize*/
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
    "CEF binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyCEF::methods,             /* tp_methods */
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

PyMethodDef VRPyCEF::methods[] = {
    {"open", (PyCFunction)VRPyCEF::open, METH_VARARGS, "Open URL" },
    {"setMaterial", (PyCFunction)VRPyCEF::setMaterial, METH_VARARGS, "Set material" },
    {"addMouse", (PyCFunction)VRPyCEF::addMouse, METH_VARARGS, "Add mouse interaction - addMouse(mouse, geo, LMB, RMB, SCRUP, SCRDOWN)" },
    {"addKeyboard", (PyCFunction)VRPyCEF::addKeyboard, METH_VARARGS, "Add keyboard device" },
    {"setResolution", (PyCFunction)VRPyCEF::setResolution, METH_VARARGS, "Set horizontal resolution W" },
    {"setAspectRatio", (PyCFunction)VRPyCEF::setAspectRatio, METH_VARARGS, "Set aspect ratio a to define the height H: H = W/a" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCEF::setResolution(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setResolution(parseFloat(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyCEF::setAspectRatio(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setAspectRatio(parseFloat(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyCEF::open(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->open(parseString(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyCEF::setMaterial(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyMaterial* mat;
    if (! PyArg_ParseTuple(args, "O", &mat)) return NULL;
    if ((PyObject*)mat == Py_None) { PyErr_SetString(err, "VRPyCEF::setMaterial, material is invalid"); return NULL; }
    self->objPtr->setMaterial(mat->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyCEF::addMouse(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev; VRPyObject* surf; int b1,b2,b3,b4;
    if (! PyArg_ParseTuple(args, "OOiiii", &dev, &surf, &b1, &b2, &b3, &b4)) return NULL;
    if ((PyObject*)dev == Py_None) { Py_RETURN_TRUE; }
    if ((PyObject*)surf == Py_None) { PyErr_SetString(err, "VRPyCEF::addMouse, surface is invalid"); return NULL; }
    self->objPtr->addMouse(dev->objPtr, surf->objPtr, b1, b2, b3, b4);
    Py_RETURN_TRUE;
}

PyObject* VRPyCEF::addKeyboard(VRPyCEF* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    if ((PyObject*)dev == Py_None) { PyErr_SetString(err, "VRPyCEF::addKeyboard, keyboard is invalid"); return NULL; }
    self->objPtr->addKeyboard(dev->objPtr);
    Py_RETURN_TRUE;
}
