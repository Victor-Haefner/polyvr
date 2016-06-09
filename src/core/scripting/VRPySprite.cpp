#include "VRPySprite.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSprite>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Sprite",             /*tp_name*/
    sizeof(VRPySprite),             /*tp_basicsize*/
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
    "VRSprite binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySprite::methods,             /* tp_methods */
    VRPySprite::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_ptr,                 /* tp_new */
};

PyMemberDef VRPySprite::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySprite::methods[] = {
    {"getText", (PyCFunction)VRPySprite::getText, METH_NOARGS, "Get label text from sprite." },
    {"getSize", (PyCFunction)VRPySprite::getSize, METH_NOARGS, "Get size of sprite." },
    {"setText", (PyCFunction)VRPySprite::setText, METH_VARARGS, "Set label text from sprite." },
    {"setSize", (PyCFunction)VRPySprite::setSize, METH_VARARGS, "Set sprite size." },
    {"setTexture", (PyCFunction)VRPySprite::setTexture, METH_VARARGS, "Set sprite texture" },
    {"webOpen", (PyCFunction)VRPySprite::webOpen, METH_VARARGS, "Open and display a website - webOpen(str uri, int width, flt ratio)" },
    {"convertToCloth", (PyCFunction)VRPySprite::convertToCloth, METH_VARARGS, "convert this Sprite to cloth (softbody)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySprite::setTexture(VRPySprite* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
	PyObject* path;
    if (! PyArg_ParseTuple(args, "O", &path)) return NULL;
	self->objPtr->setTexture( PyString_AsString(path) );
    Py_RETURN_TRUE;
}

PyObject* VRPySprite::webOpen(VRPySprite* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
	PyObject* uri; int res; float ratio;
    if (! PyArg_ParseTuple(args, "Oif", &uri, &res, &ratio)) return NULL;
	self->objPtr->webOpen( PyString_AsString(uri), res, ratio);
    Py_RETURN_TRUE;
}

PyObject* VRPySprite::getText(VRPySprite* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
	return PyString_FromString(self->objPtr->getLabel().c_str());
}

PyObject* VRPySprite::getSize(VRPySprite* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
	return toPyTuple(self->objPtr->getSize());
}

PyObject* VRPySprite::setSize(VRPySprite* self, PyObject* args) {
    float x,y; x=y=0;
    if (! PyArg_ParseTuple(args, "ff", &x, &y)) return NULL;

    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }

    OSG::VRSpritePtr s = (OSG::VRSpritePtr) self->objPtr;
    s->setSize(x,y);

    Py_RETURN_TRUE;
}

PyObject* VRPySprite::setText(VRPySprite* self, PyObject* args) {
    PyObject* _text = NULL;
    if (! PyArg_ParseTuple(args, "O", &_text)) return NULL;
    string text = PyString_AsString(_text);

    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }

    OSG::VRSpritePtr s = (OSG::VRSpritePtr) self->objPtr;
    s->setLabel(text);

    Py_RETURN_TRUE;
}

PyObject* VRPySprite::convertToCloth(VRPySprite* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyTransform::convertToCloth: C Object is invalid"); return NULL; }
    self->objPtr->getPhysics()->setDynamic(true);
    self->objPtr->getPhysics()->setShape("Cloth");
    self->objPtr->getPhysics()->setSoft(true);
    self->objPtr->getPhysics()->setPhysicalized(true);
    Py_RETURN_TRUE;
}

