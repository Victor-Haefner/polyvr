#include "VRPyObjectManager.h"
#include "VRPyBaseT.h"
#include "VRPyObject.h"
#include "VRPyTransform.h"
#include "VRPyTypeCaster.h"
#include "VRPyPose.h"

using namespace OSG;

simpleVRPyType(ObjectManager, New_VRObjects_unnamed_ptr);

PyMethodDef VRPyObjectManager::methods[] = {
    {"add", (PyCFunction)VRPyObjectManager::add, METH_VARARGS, "Add a copy of the passed object to the managed object pool and return it - obj add( obj ) " },
    {"copy", (PyCFunction)VRPyObjectManager::copy, METH_VARARGS, "Add a copy of the passed object to the managed object pool and return it - obj copy( str, pose ) " },
    {"clear", (PyCFunction)VRPyObjectManager::clear, METH_NOARGS, "Clear objects - clear() " },
    {"remove", (PyCFunction)VRPyObjectManager::remove, METH_VARARGS, "Remove an object - remove( object ) " },
    {"addTemplate", (PyCFunction)VRPyObjectManager::addTemplate, METH_VARARGS, "Add template - addTemplate( int | name ) " },
    {"getTemplate", (PyCFunction)VRPyObjectManager::getTemplate, METH_VARARGS, "Return all template objects - obj getTemplate( str ) " },
    {"getCatalog", (PyCFunction)VRPyObjectManager::getCatalog, METH_VARARGS, "Return all template objects - [ obj ] getCatalog() " },
    {"updateObject", (PyCFunction)VRPyObjectManager::updateObject, METH_VARARGS, "Update object data - updateObject( obj ) " },
    {NULL}  /* Sentinel */
};

PyObject* VRPyObjectManager::updateObject(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->updateObject( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyObjectManager::add(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    if (isNone((PyObject*)o)) { setErr("Object passed is None"); return NULL; }
    return VRPyTypeCaster::cast( self->objPtr->add( o->objPtr ) );
}

PyObject* VRPyObjectManager::copy(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* s = 0;
    VRPyPose* p = 0;
    if (! PyArg_ParseTuple(args, "sO", &s, &p)) return NULL;
    return VRPyTypeCaster::cast( self->objPtr->copy( s?s:"", p->objPtr ) );
}

PyObject* VRPyObjectManager::remove(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->rem(o->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyObjectManager::getTemplate(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* s = 0;
    if (! PyArg_ParseTuple(args, "s", &s)) return NULL;
    return VRPyTypeCaster::cast( self->objPtr->getTemplate(s?s:"") );
}

PyObject* VRPyObjectManager::clear(VRPyObjectManager* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyObjectManager::addTemplate(VRPyObjectManager* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o = 0;
    const char* s = 0;
    if (! PyArg_ParseTuple(args, "O|s", &o, &s)) return NULL;
    self->objPtr->addTemplate(o->objPtr, s?s:"");
    Py_RETURN_TRUE;
}

PyObject* VRPyObjectManager::getCatalog(VRPyObjectManager* self) {
    if (!self->valid()) return NULL;
    auto objs = self->objPtr->getCatalog();
    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    return li;
}



