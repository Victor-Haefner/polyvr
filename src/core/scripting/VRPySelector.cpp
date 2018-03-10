#include "VRPySelector.h"
#include "VRPySelection.h"
#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Selector, New_ptr);

PyMethodDef VRPySelector::methods[] = {
    {"setColor", (PyCFunction)VRPySelector::setColor, METH_VARARGS, "Set the color of the selection - setColor([f,f,f])" },
    {"deselect", (PyCFunction)VRPySelector::clear, METH_VARARGS, "Deselect object - deselect()" },
    {"select", PyWrapOpt(Selector, select, "Select object - select( obj, add, recursive )", "0|1", void, VRObjectPtr, bool, bool ) },
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
    self->objPtr->setBorder(w,s);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::setColor(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setColor(Vec3f(parseVec3d(args)));
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::select(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* obj = 0;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    if (!isNone((PyObject*)obj)) self->objPtr->select(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::set(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySelection* obj = 0;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    if (!isNone((PyObject*)obj)) self->objPtr->select(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::add(VRPySelector* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySelection* obj;
    parseObject(args, obj);
    if (obj == 0) return NULL;
    self->objPtr->add(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::clear(VRPySelector* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::update(VRPySelector* self) {
    if (!self->valid()) return NULL;
    self->objPtr->update();
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::getSelection(VRPySelector* self) {
    if (!self->valid()) return NULL;
    return VRPySelection::fromSharedPtr( self->objPtr->getSelection() );
}
