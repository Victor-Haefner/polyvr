#include "VRPyPolygon.h"
#include "VRPyBaseT.h"

using namespace OSG;

newPyType(polygon, Polygon, New_ptr);

PyMethodDef VRPyPolygon::methods[] = {
    {"addPoint", (PyCFunction)VRPyPolygon::addPoint, METH_VARARGS, "Add a point - addPoint([x,y,z])" },
    {"getPoint", (PyCFunction)VRPyPolygon::getPoint, METH_VARARGS, "Get a point - [x,y,z] getPoint( int i )" },
    {"getPoints", (PyCFunction)VRPyPolygon::getPoints, METH_NOARGS, "Get the list of points - [[x,y,z]] getPoints()" },
    {"getConvexHull", (PyCFunction)VRPyPolygon::getConvexHull, METH_NOARGS, "Get the convex hull - polygon getConvexHull()" },
    {"close", (PyCFunction)VRPyPolygon::close, METH_NOARGS, "Close the polygon - close()" },
    {"size", (PyCFunction)VRPyPolygon::size, METH_NOARGS, "Get the number of points - int size()" },
    {"set", (PyCFunction)VRPyPolygon::set, METH_VARARGS, "Set the polygon from a list of points - set( [[x,y,z]] )" },
    {"clear", (PyCFunction)VRPyPolygon::clear, METH_NOARGS, "Clear all points - clear()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPolygon::getConvexHull(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    return VRPyPolygon::fromObject( self->objPtr->getConvexHull() );
}

PyObject* VRPyPolygon::addPoint(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "O", &v)) return NULL;
    self->objPtr->addPoint( parseVec2fList(v) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::getPoint(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return toPyTuple( self->objPtr->getPoint(i) );
}

PyObject* VRPyPolygon::getPoints(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    auto vec = self->objPtr->get();
    PyObject* res = PyList_New(vec.size());
    for (uint i=0; i<vec.size(); i++) PyList_SetItem(res, i, toPyTuple(vec[i]));
    return res;
}

PyObject* VRPyPolygon::close(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    self->objPtr->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::size(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->size() );
}

PyObject* VRPyPolygon::set(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "O", &v)) return NULL;
    vector<OSG::Vec2f> vec;
    for (int i=0; i<pySize(v); i++) vec.push_back( parseVec2fList( PyList_GetItem(v,i) ) );
    self->objPtr->set(vec);
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::clear(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}


