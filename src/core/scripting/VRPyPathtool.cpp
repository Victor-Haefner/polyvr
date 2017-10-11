#include "VRPyPathtool.h"
#include "VRPyMaterial.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyGraph.h"
#include "VRPyBaseT.h"
#include "VRPyStroke.h"
#include "VRPyPose.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Pathtool, New_ptr);

PyMethodDef VRPyPathtool::methods[] = {
    {"newPath", (PyCFunction)VRPyPathtool::newPath, METH_VARARGS, "Add a new path - path newPath(device, anchor | int resolution, bool doControlHandles )" },
    {"remPath", (PyCFunction)VRPyPathtool::remPath, METH_VARARGS, "Remove a path - remPath(path)" },
    {"extrude", (PyCFunction)VRPyPathtool::extrude, METH_VARARGS, "Extrude a path - handle extrude(device, path)" },
    {"addPath", (PyCFunction)VRPyPathtool::addPath, METH_VARARGS, "Add a path and add resulting stroke to object - addPath(path, object)" },
    {"select", (PyCFunction)VRPyPathtool::select, METH_VARARGS, "Select handle - select(handle)" },
    {"deselect", (PyCFunction)VRPyPathtool::deselect, METH_NOARGS, "Deselect anything previously selected - deselect()" },
    {"setVisuals", (PyCFunction)VRPyPathtool::setVisuals, METH_VARARGS, "Set the tool visibility - setVisuals(bool)\n     setVisuals(bool stroke, bool handles)" },
    {"getPaths", (PyCFunction)VRPyPathtool::getPaths, METH_VARARGS, "Return all paths or paths connected to handle - [path] getPaths( | handle )" },
    {"getPath", (PyCFunction)VRPyPathtool::getPath, METH_VARARGS, "Return path between handles h1 and h2 - [path] getPath( handle h1, handle h2 )" },
    {"getHandle", (PyCFunction)VRPyPathtool::getHandle, METH_VARARGS, "Return a handle by node ID - handle getHandle( int ID )" },
    {"getHandles", (PyCFunction)VRPyPathtool::getHandles, METH_VARARGS, "Return a list of paths handles - [handle] getHandles(path)" },
    {"getStroke", (PyCFunction)VRPyPathtool::getStroke, METH_VARARGS, "Return the stroke object - stroke getStroke(path)" },
    {"update", (PyCFunction)VRPyPathtool::update, METH_NOARGS, "Update the tool - update()" },
    {"clear", (PyCFunction)VRPyPathtool::clear, METH_VARARGS, "Clear all path nodes - clear(path)" },
    {"setHandleGeometry", (PyCFunction)VRPyPathtool::setHandleGeometry, METH_VARARGS, "Replace the default handle geometry - setHandleGeometry( geo )" },
    {"getPathMaterial", (PyCFunction)VRPyPathtool::getPathMaterial, METH_NOARGS, "Get the material used for paths geometry - getPathMaterial()" },
    {"setGraph", (PyCFunction)VRPyPathtool::setGraph, METH_VARARGS, "Setup from graph - setGraph( graph )" },
    {"addNode", (PyCFunction)VRPyPathtool::addNode, METH_VARARGS, "Add node - int addNode( pose )" },
    {"removeNode", (PyCFunction)VRPyPathtool::removeNode, METH_VARARGS, "Remove node by id - removeNode( int )" },
    {"getNodeID", (PyCFunction)VRPyPathtool::getNodeID, METH_VARARGS, "Return node ID from handle - getNodeID( handle )" },
    {"connect", (PyCFunction)VRPyPathtool::connect, METH_VARARGS, "Connect two nodes by id, using optional normals - connect( id1, id2 | n1, n2, doHandles, addArrow)" },
    {"disconnect", (PyCFunction)VRPyPathtool::disconnect, METH_VARARGS, "Disconnect two nodes - disconnect( id1, id2 )" },
    {"setProjectionGeometry", (PyCFunction)VRPyPathtool::setProjectionGeometry, METH_VARARGS, "Set an object to project handles onto - setProjectionGeometry( object )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathtool::setProjectionGeometry(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* g = 0;
    if (! PyArg_ParseTuple(args, "O:setProjectionGeometry", &g)) return NULL;
    self->objPtr->setProjectionGeometry( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::getNodeID(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* g = 0;
    if (! PyArg_ParseTuple(args, "O:getNodeID", &g)) return NULL;
    return PyInt_FromLong( self->objPtr->getNodeID( g->objPtr ) );
}

PyObject* VRPyPathtool::setGraph(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* g = 0;
    if (! PyArg_ParseTuple(args, "O:setGraph", &g)) return NULL;
    if (g) self->objPtr->setGraph( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::disconnect(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i1 = 0;
    int i2 = 0;
    if (! PyArg_ParseTuple(args, "ii:disconnect", &i1, &i2)) return NULL;
    self->objPtr->disconnect( i1, i2 );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::connect(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i1 = 0;
    int i2 = 0;
    int doHandles = 1;
    int addArrow = 0;
    PyObject* n1 = 0;
    PyObject* n2 = 0;
    if (! PyArg_ParseTuple(args, "ii|OOii:connect", &i1, &i2, &n1, &n2, &doHandles, &addArrow)) return NULL;
    if (n1 && n2) self->objPtr->connect( i1, i2, doHandles, addArrow, parseVec3dList(n1), parseVec3dList(n2) );
    else self->objPtr->connect( i1, i2 );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::removeNode(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i:removeNode", &i)) return NULL;
    self->objPtr->remNode( i );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::getHandle(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i:getHandle", &i)) return NULL;
    return VRPyTypeCaster::cast( self->objPtr->getHandle( i ) );
}

PyObject* VRPyPathtool::addNode(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* g = 0;
    if (! PyArg_ParseTuple(args, "O:addNode", &g)) return NULL;
    return PyInt_FromLong( self->objPtr->addNode( g->objPtr ) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::clear(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "|O:clear", &p)) return NULL;
    if (p) self->objPtr->clear(p->objPtr);
    else   self->objPtr->clear();
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

PyObject* VRPyPathtool::getPaths(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* h = 0;
    if (!PyArg_ParseTuple(args, "|O", &h)) return NULL;
    auto paths = self->objPtr->getPaths(h ? h->objPtr : 0);
    PyObject* li = PyList_New(paths.size());
    for (uint i=0; i<paths.size(); i++) {
        PyList_SetItem(li, i, VRPyPath::fromSharedPtr(paths[i]));
    }
    return li;
}

PyObject* VRPyPathtool::getPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry *h1, *h2;
    if (!PyArg_ParseTuple(args, "OO", &h1, &h2)) return NULL;
    auto p = self->objPtr->getPath(h1->objPtr, h2->objPtr);
    return VRPyPath::fromSharedPtr(p);
}

PyObject* VRPyPathtool::getStroke(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = 0;
    if (!PyArg_ParseTuple(args, "O:getStroke", &p)) return NULL;
    return VRPyStroke::fromSharedPtr( self->objPtr->getStroke(p->objPtr) );
}

PyObject* VRPyPathtool::getHandles(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;

    VRPyPath* p = 0;
    if (! PyArg_ParseTuple(args, "|O:getHandles", &p)) return NULL;
    pathPtr pa = 0;
    if (p) pa = p->objPtr;

    vector<VRGeometryPtr> objs = self->objPtr->getHandles(pa);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyGeometry::fromSharedPtr(objs[i]));
    }

    return li;
}

PyObject* VRPyPathtool::setVisuals(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int b1, b2 = 0;
    if (! PyArg_ParseTuple(args, "i|i:setVisuals", &b1, &b2)) return NULL;
    if (pySize(args) == 1) b2 = b1;
    self->objPtr->setVisuals( b1, b2 );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::select(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* o;
    if (! PyArg_ParseTuple(args, "O:select", &o)) return NULL;
    if (VRPyGeometry::check(o)) self->objPtr->select( ((VRPyGeometry*)o)->objPtr );
    if (VRPyPath::check(o)) self->objPtr->select( ((VRPyPath*)o)->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::deselect(VRPyPathtool* self) {
    if (!self->valid()) return NULL;
    self->objPtr->deselect();
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::addPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p; VRPyObject* obj;
    if (! PyArg_ParseTuple(args, "OO:addPath", &p, &obj)) return NULL;
    VRObjectPtr anchor;
    if (!isNone((PyObject*)obj)) anchor = obj->objPtr;
    self->objPtr->addPath( p->objPtr, anchor );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::newPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev; VRPyObject* obj;
    int res = 10; int doCH = 0;
    if (! PyArg_ParseTuple(args, "OO|ii:newPath", &dev, &obj, &res, &doCH)) return NULL;
    VRDevicePtr d = 0;
    if (!isNone((PyObject*)dev)) d = dev->objPtr;
    auto p = self->objPtr->newPath( d, obj->objPtr, res, doCH );
    return VRPyPath::fromSharedPtr(p);
}

PyObject* VRPyPathtool::remPath(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPath* p = (VRPyPath*)parseObject(args);
    self->objPtr->remPath( p->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::extrude(VRPyPathtool* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyDevice* dev; VRPyPath* p;
    if (! PyArg_ParseTuple(args, "OO:extrude", &dev, &p)) return NULL;
    VRDevicePtr d = 0;
    if (!isNone((PyObject*)dev)) d = dev->objPtr;
    return VRPyGeometry::fromSharedPtr( self->objPtr->extrude( d, p->objPtr) );
}

