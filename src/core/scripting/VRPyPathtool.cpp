#include "VRPyPathtool.h"
#include "VRPyMaterial.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyGraph.h"
#include "VRPyBaseT.h"
#include "VRPyStroke.h"

using namespace OSG;

simpleVRPyType(Pathtool, New_ptr);

PyMethodDef VRPyPathtool::methods[] = {
    {"newPath", (PyCFunction)VRPyPathtool::newPath, METH_VARARGS, "Add a new path - path newPath(device, anchor)" },
    {"remPath", (PyCFunction)VRPyPathtool::remPath, METH_VARARGS, "Remove a path - remPath(path)" },
    {"extrude", (PyCFunction)VRPyPathtool::extrude, METH_VARARGS, "Extrude a path - handle extrude(device, path)" },
    {"addPath", (PyCFunction)VRPyPathtool::addPath, METH_VARARGS, "Add a path - addPath(path, object)" },
    {"select", (PyCFunction)VRPyPathtool::select, METH_VARARGS, "Select handle - select(handle)" },
    {"setVisible", (PyCFunction)VRPyPathtool::setVisible, METH_VARARGS, "Set the tool visibility - setVisible(bool)\n     setVisible(bool stroke, bool handles)" },
    {"getPaths", (PyCFunction)VRPyPathtool::getPaths, METH_NOARGS, "Return alist of all paths - [path] getPaths()" },
    {"getHandles", (PyCFunction)VRPyPathtool::getHandles, METH_VARARGS, "Return a list of paths handles - [handle] getHandles(path)" },
    {"getStroke", (PyCFunction)VRPyPathtool::getStroke, METH_VARARGS, "Return the stroke object - stroke getStroke(path)" },
    {"update", (PyCFunction)VRPyPathtool::update, METH_NOARGS, "Update the tool - update()" },
    {"clear", (PyCFunction)VRPyPathtool::clear, METH_VARARGS, "Clear all path nodes - clear(path)" },
    {"setHandleGeometry", (PyCFunction)VRPyPathtool::setHandleGeometry, METH_VARARGS, "Replace the default handle geometry - setHandleGeometry( geo )" },
    {"getPathMaterial", (PyCFunction)VRPyPathtool::getPathMaterial, METH_NOARGS, "Get the material used for paths geometry - getPathMaterial()" },
    {"setGraph", (PyCFunction)VRPyPathtool::setGraph, METH_NOARGS, "Setup from graph - setGraph( graph )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathtool::setGraph(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* g = 0;
    if (! PyArg_ParseTuple(args, "O:setGraph", &g)) return NULL;
    self->objPtr->setGraph( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::clear(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "|O:clear", &p)) return NULL;
    path* pa = p ? p->obj : 0;
    self->objPtr->clear(pa);
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::setHandleGeometry(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    if (! PyArg_ParseTuple(args, "O:setHandleGeometry", &geo)) return NULL;
    self->objPtr->setHandleGeometry(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::getPathMaterial(VRPyPathtool* self) {
    if (!self->valid()) return NULL;
    return VRPyMaterial::fromSharedPtr( self->objPtr->getPathMaterial() );
}

PyObject* VRPyPathtool::update(VRPyPathtool* self) {
    if (!self->valid()) return NULL;
    self->objPtr->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::getPaths(VRPyPathtool* self) {
    if (!self->valid()) return NULL;
    vector<path*> objs = self->objPtr->getPaths();

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyPath::fromPtr(objs[i]));
    }

    return li;
}

PyObject* VRPyPathtool::getStroke(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "O:getStroke", &p)) return NULL;
    return VRPyStroke::fromSharedPtr( self->objPtr->getStroke(p->obj) );
}

PyObject* VRPyPathtool::getHandles(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;

    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "|O:getHandles", &p)) return NULL;
    path* pa = 0;
    if (p) pa = p->obj;

    vector<VRGeometryPtr> objs = self->objPtr->getHandles(pa);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyGeometry::fromSharedPtr(objs[i]));
    }

    return li;
}

PyObject* VRPyPathtool::setVisible(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int b1, b2 = 0;
    if (! PyArg_ParseTuple(args, "i|i:setVisible", &b1, &b2)) return NULL;
    if (pySize(args) == 1) b2 = b1;
    self->objPtr->setVisible( b1, b2 );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::select(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* obj;
    if (! PyArg_ParseTuple(args, "O:select", &obj)) return NULL;
    self->objPtr->select( obj->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::addPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p; VRPyObject* obj;
    if (! PyArg_ParseTuple(args, "OO:addPath", &p, &obj)) return NULL;
    self->objPtr->addPath( p->obj, obj->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::newPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev; VRPyObject* obj; int res = 10;
    if (! PyArg_ParseTuple(args, "OO|i:newPath", &dev, &obj, &res)) return NULL;
    VRDevicePtr d = 0;
    if (!isNone((PyObject*)dev)) d = dev->objPtr;
    path* p = self->objPtr->newPath( d, obj->objPtr, res );
    return VRPyPath::fromPtr(p);
}

PyObject* VRPyPathtool::remPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = (VRPyPath*)parseObject(args);
    self->objPtr->remPath( p->obj );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::extrude(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev; VRPyPath* p;
    if (! PyArg_ParseTuple(args, "OO:extrude", &dev, &p)) return NULL;
    VRDevicePtr d = 0;
    if (!isNone((PyObject*)dev)) d = dev->objPtr;
    return VRPyGeometry::fromSharedPtr( self->objPtr->extrude( d, p->obj) );
}

