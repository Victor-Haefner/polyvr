#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPySelector.h"
#include "VRPyGeometry.h"
#include "VRPyTypeCaster.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ConstructionKit, New_ptr);

PyMethodDef VRPyConstructionKit::methods[] = {
    {"clear", (PyCFunction)VRPyConstructionKit::clear, METH_NOARGS, "Clear everything - clear()" },
    {"getSnappingEngine", (PyCFunction)VRPyConstructionKit::getSnappingEngine, METH_NOARGS, "Get internal snapping engine - getSnappingEngine()" },
    {"getSelector", (PyCFunction)VRPyConstructionKit::getSelector, METH_NOARGS, "Get internal selector - getSelector(obj)" },
    {"getObjects", (PyCFunction)VRPyConstructionKit::getObjects, METH_NOARGS, "Get all objects - [obj] getObjects()" },
    {"addAnchorType", (PyCFunction)VRPyConstructionKit::addAnchorType, METH_VARARGS, "Add new anchor type - addAnchorType(size, color)" },
    {"addObjectAnchor", (PyCFunction)VRPyConstructionKit::addObjectAnchor, METH_VARARGS, "Add anchor to object - addObjectAnchor(obj, int anchor, position, flt radius)" },
    {"addObject", (PyCFunction)VRPyConstructionKit::addObject, METH_VARARGS, "Get internal selector - addObject(obj)" },
    {"remObject", (PyCFunction)VRPyConstructionKit::remObject, METH_VARARGS, "Get internal selector - remObject(obj)" },
    {"breakup", (PyCFunction)VRPyConstructionKit::breakup, METH_VARARGS, "Split an object from the system - breakup(obj)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyConstructionKit::getSnappingEngine(VRPyConstructionKit* self) { return VRPySnappingEngine::fromSharedPtr(self->objPtr->getSnappingEngine()); }
PyObject* VRPyConstructionKit::getSelector(VRPyConstructionKit* self) { return VRPySelector::fromSharedPtr(self->objPtr->getSelector()); }
PyObject* VRPyConstructionKit::clear(VRPyConstructionKit* self) { self->objPtr->clear(); Py_RETURN_TRUE; }

PyObject* VRPyConstructionKit::getObjects(VRPyConstructionKit* self) {
    if (!self->valid()) return NULL;

    auto objs = self->objPtr->getObjects();
    PyObject* res = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(res, i, VRPyTypeCaster::cast(objs[i]));
    }

    return res;
}

PyObject* VRPyConstructionKit::breakup(VRPyConstructionKit* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::VRGeometryPtr geo = 0;
    if (!VRPyGeometry::parse(args, &geo)) return NULL;
    self->objPtr->breakup(geo);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstructionKit::addAnchorType(VRPyConstructionKit* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float f; PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "fO", &f, &o)) return NULL;
    return PyInt_FromLong( self->objPtr->addAnchorType(f, Vec3f(parseVec3dList(o))) );
}

PyObject* VRPyConstructionKit::addObjectAnchor(VRPyConstructionKit* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* g; PyObject* p;
    int a; float d;
    if (! PyArg_ParseTuple(args, "OiOf", &g, &a, &p, &d)) return NULL;
    auto anc = self->objPtr->addObjectAnchor(g->objPtr, a, parseVec3dList(p), d);
    return VRPyGeometry::fromSharedPtr(anc);
}

PyObject* VRPyConstructionKit::addObject(VRPyConstructionKit* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::VRGeometryPtr geo = 0;
    if (!VRPyGeometry::parse(args, &geo)) return NULL;
    self->objPtr->addObject(geo);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstructionKit::remObject(VRPyConstructionKit* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::VRGeometryPtr geo = 0;
    if (!VRPyGeometry::parse(args, &geo)) return NULL;
    self->objPtr->remObject(geo);
    Py_RETURN_TRUE;
}


