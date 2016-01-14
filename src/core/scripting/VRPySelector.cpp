#include "VRPySelector.h"
#include "VRPySelection.h"
#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSelector>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Selector",             /*tp_name*/
    sizeof(VRPySelector),             /*tp_basicsize*/
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
    "ColorChooser binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySelector::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef VRPySelector::methods[] = {
    {"setColor", (PyCFunction)VRPySelector::setColor, METH_VARARGS, "Set the color of the selection - setColor([f,f,f])" },
    {"deselect", (PyCFunction)VRPySelector::clear, METH_VARARGS, "Deselect object - deselect()" },
    {"select", (PyCFunction)VRPySelector::select, METH_VARARGS, "Select object - select( obj )" },
    {"update", (PyCFunction)VRPySelector::update, METH_NOARGS, "Update selection visualisation - update()" },
    {"set", (PyCFunction)VRPySelector::set, METH_VARARGS, "Set selection - set( selection )" },
    {"add", (PyCFunction)VRPySelector::add, METH_VARARGS, "Add to selection - add( selection )" },
    {"clear", (PyCFunction)VRPySelector::clear, METH_NOARGS, "Clear selection - deselect()" },
    {"getSelection", (PyCFunction)VRPySelector::getSelection, METH_NOARGS, "Return the selected object - object getSelection()" },
    {"setBorder", (PyCFunction)VRPySelector::setBorder, METH_VARARGS, "Set the border with and toggles smoothness - setBorder(int width, bool smooth)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySelector::setBorder(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int w; int s = 1;
    if (! PyArg_ParseTuple(args, "i|i:setBorder", &w, &s)) return NULL;
    self->obj->setBorder(w,s);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::setColor(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->obj->setColor(parseVec3f(args));
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::select(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* obj = 0;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    if (!isNone((PyObject*)obj)) self->obj->select(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::set(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySelection* obj = 0;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    if (!isNone((PyObject*)obj)) self->obj->select(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::add(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySelection* obj;
    parseObject(args, obj);
    if (obj == 0) return NULL;
    self->obj->add(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::clear(VRPySelector* self) {
    if (!self->valid()) return NULL;
    self->obj->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::update(VRPySelector* self) {
    if (!self->valid()) return NULL;
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::getSelection(VRPySelector* self) {
    if (!self->valid()) return NULL;
    return VRPySelection::fromSharedPtr( self->obj->getSelection() );
}
