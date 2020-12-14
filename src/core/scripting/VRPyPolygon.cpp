#include "VRPyPolygon.h"
#include "VRPyMath.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRPolygon& p) { return VRPyPolygon::fromObject(p); }
template<> PyObject* VRPyTypeCaster::cast(const Frustum& p) { return VRPyFrustum::fromObject(p); }
template<> bool toValue(PyObject* o, VRPolygon& v) { if (!VRPyPolygon::check(o)) return 0; v = *((VRPyPolygon*)o)->objPtr; return 1; }
template<> bool toValue(PyObject* o, Frustum& v) { if (!VRPyFrustum::check(o)) return 0; v = *((VRPyFrustum*)o)->objPtr; return 1; }

newPyType(VRPolygon, Polygon, New_ptr);
newPyType(Frustum, Frustum, New_ptr);

PyMethodDef VRPyFrustum::methods[] = {
    {"isInside", PyWrap2(Frustum, isInside, "Check if point in frustum", bool, Vec3d) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPolygon::methods[] = {
    {"addPoint", (PyCFunction)VRPyPolygon::addPoint, METH_VARARGS, "Add a point - addPoint([x,y])" },
    {"getPoint", (PyCFunction)VRPyPolygon::getPoint, METH_VARARGS, "Get a point - [x,y] getPoint( int i )" },
    {"getPoints", (PyCFunction)VRPyPolygon::getPoints, METH_NOARGS, "Get the list of points - [[x,y]] getPoints()" },
    {"getConvexHull", (PyCFunction)VRPyPolygon::getConvexHull, METH_NOARGS, "Get the convex hull - VRPolygon getConvexHull()" },
    {"close", (PyCFunction)VRPyPolygon::close, METH_NOARGS, "Close the VRPolygon - close()" },
    {"size", (PyCFunction)VRPyPolygon::size, METH_NOARGS, "Get the number of points - int size()" },
    {"set", (PyCFunction)VRPyPolygon::set, METH_VARARGS, "Set the VRPolygon from a list of points - set( [[x,y]] )" },
    {"clear", (PyCFunction)VRPyPolygon::clear, METH_NOARGS, "Clear all points - clear()" },
    {"getRandomPoints", (PyCFunction)VRPyPolygon::getRandomPoints, METH_VARARGS, "Clear all points - getRandomPoints( | float density, float padding)" },
    {"isInside", PyWrap(Polygon, isInside, "Check if point is inside polygon", bool, Vec2d) },
    {"gridSplit", PyWrap(Polygon, gridSplit, "Split the polygon using a virtual grid layout", vector< VRPolygonPtr >, double) },
    {"reverseOrder", PyWrap(Polygon, reverseOrder, "Reverse the order of the points", void) },
    {"translate", PyWrap(Polygon, translate, "Translate all points", void, Vec3d) },
    {"removeDoubles", PyWrapOpt(Polygon, removeDoubles, "Remove doubles", "0.001", void, float) },
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
    self->objPtr->addPoint( parseVec2dList(v) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::getPoint(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return toPyObject( self->objPtr->getPoint(i) );
}

PyObject* VRPyPolygon::getRandomPoints(VRPyPolygon* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float d = 10;
    float p = 0;
    float s = 0.5;
    if (! PyArg_ParseTuple(args, "|fff", &d, &p, &s)) return NULL;
    auto vec = self->objPtr->getRandomPoints(d,p,s);
    PyObject* res = PyList_New(vec.size());
    for (unsigned int i=0; i<vec.size(); i++) PyList_SetItem(res, i, toPyObject(vec[i]));
    return res;
}

PyObject* VRPyPolygon::getPoints(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    auto vec = self->objPtr->get();
    PyObject* res = PyList_New(vec.size());
    for (unsigned int i=0; i<vec.size(); i++) PyList_SetItem(res, i, toPyObject(vec[i]));
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
    vector<OSG::Vec2d> vec;
    for (int i=0; i<pySize(v); i++) vec.push_back( parseVec2dList( PyList_GetItem(v,i) ) );
    self->objPtr->set(vec);
    Py_RETURN_TRUE;
}

PyObject* VRPyPolygon::clear(VRPyPolygon* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}


