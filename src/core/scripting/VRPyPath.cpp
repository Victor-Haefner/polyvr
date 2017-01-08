#include "VRPyPath.h"
#include "VRPyBaseT.h"
#include "VRPyPose.h"

using namespace OSG;

newPyType( path, Path, New_ptr);

PyMethodDef VRPyPath::methods[] = {
    {"set", (PyCFunction)VRPyPath::set, METH_VARARGS, "Set the path - set(start pos, start dir, end pos, end dir, steps) \n\tset(start pos, start dir, start up, end pos, end dir, end up, steps)" },
    {"setStart", (PyCFunction)VRPyPath::setStartPoint, METH_VARARGS, "Set the path start point" },
    {"setEnd", (PyCFunction)VRPyPath::setEndPoint, METH_VARARGS, "Set the path end point" },
    {"getStart", (PyCFunction)VRPyPath::getStartPoint, METH_NOARGS, "Get the path start point" },
    {"getEnd", (PyCFunction)VRPyPath::getEndPoint, METH_NOARGS, "Get the path end point" },
    {"invert", (PyCFunction)VRPyPath::invert, METH_NOARGS, "Invert start && end point of path" },
    {"compute", (PyCFunction)VRPyPath::compute, METH_VARARGS, "Compute path with given resolution, allways call this after adding all path points - compute( int resolution )" },
    {"update", (PyCFunction)VRPyPath::update, METH_NOARGS, "Update path" },
    {"addPoint", (PyCFunction)VRPyPath::addPoint, METH_VARARGS, "Add a point to the path - int addPoint( | vec3 pos, vec3 dir, vec3 col, vec3 up)" },
    {"getPose", (PyCFunction)VRPyPath::getPose, METH_VARARGS, "Return the pose at the path length t, t on the interval between point i and j - pose getPose( float t | int i, int j)" },
    {"getPoints", (PyCFunction)VRPyPath::getPoints, METH_NOARGS, "Return a list of the path points - [pos, dir, col, up] getPoints()" },
    {"close", (PyCFunction)VRPyPath::close, METH_NOARGS, "Close the path - close()" },
    {"getPositions", (PyCFunction)VRPyPath::getPositions, METH_NOARGS, "Return the positions from the computed path - [[x,y,z]] getPositions()" },
    {"getDirections", (PyCFunction)VRPyPath::getDirections, METH_NOARGS, "Return the directions from the computed path - [[x,y,z]] getDirections()" },
    {"getUpVectors", (PyCFunction)VRPyPath::getUpVectors, METH_NOARGS, "Return the up vectors from the computed path - [[x,y,z]] getUpVectors()" },
    {"getColors", (PyCFunction)VRPyPath::getColors, METH_NOARGS, "Return the colors from the computed path - [[x,y,z]] getColors()" },
    {"getSize", (PyCFunction)VRPyPath::getSize, METH_NOARGS, "Return the number of path nodes - int getSize()" },
    {"getLength", (PyCFunction)VRPyPath::getLength, METH_NOARGS, "Return the approximated path length - float getLength()" },
    {"getDistance", (PyCFunction)VRPyPath::getDistance, METH_VARARGS, "Return the distance from point to path - float getDistance( [x,y,z] )" },
    {"approximate", (PyCFunction)VRPyPath::approximate, METH_VARARGS, "Convert the cubic bezier spline in a quadratic or linear one (currently only quadratic) - approximate(int degree)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPath::getDistance(VRPyPath* self, PyObject *args) {
    if (!self->valid()) return NULL;
    auto p = parseVec3f( args );
    return PyFloat_FromDouble( self->objPtr->getDistance(p) );
}

PyObject* VRPyPath::approximate(VRPyPath* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    self->objPtr->approximate(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::getPose(VRPyPath* self, PyObject *args) {
    if (!self->valid()) return NULL;
    float t = 0;
    int i = 0;
    int j = 0;
    if (! PyArg_ParseTuple(args, "f|ii", &t, &i, &j)) return NULL;
    return VRPyPose::fromObject( self->objPtr->getPose(t, i, j) );
}

PyObject* VRPyPath::getLength(VRPyPath* self) {
	if (!self->valid()) return NULL;
    return PyFloat_FromDouble( self->objPtr->getLength() );
}

PyObject* VRPyPath::getSize(VRPyPath* self) {
	if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->size() );
}

PyObject* VRPyPath::getPoints(VRPyPath* self) {
	if (!self->valid()) return NULL;
    auto pnts = self->objPtr->getPoints();
    if (pnts.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pnts.size());
    for (uint i=0; i<pnts.size(); i++) {
        PyObject* pnt = PyList_New(4);
        PyList_SetItem(pnt, 0, toPyTuple(pnts[i].p));
        PyList_SetItem(pnt, 1, toPyTuple(pnts[i].n));
        PyList_SetItem(pnt, 2, toPyTuple(pnts[i].c));
        PyList_SetItem(pnt, 3, toPyTuple(pnts[i].u));
        PyList_SetItem(res, i, pnt);
    }
    return res;
}

PyObject* VRPyPath::getPositions(VRPyPath* self) {
	if (!self->valid()) return NULL;
    auto pos = self->objPtr->getPositions();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getDirections(VRPyPath* self) {
	if (!self->valid()) return NULL;
    auto pos = self->objPtr->getDirections();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getUpVectors(VRPyPath* self) {
	if (!self->valid()) return NULL;
    auto pos = self->objPtr->getUpvectors();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getColors(VRPyPath* self) {
	if (!self->valid()) return NULL;
    auto pos = self->objPtr->getColors();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::set(VRPyPath* self, PyObject* args) {
	if (!self->valid()) return NULL;

    int i;
    PyObject *p1, *p2, *n1, *n2, *u1 = 0, *u2 = 0;
    if (PyList_GET_SIZE(args) <= 5) {
        if (! PyArg_ParseTuple(args, "OOOOi", &p1, &n1, &p2, &n2, &i)) return NULL;
    } else if (! PyArg_ParseTuple(args, "OOOOOOi", &p1, &n1, &u1, &p2, &n2, &u2, &i)) return NULL;

    OSG::Vec3f c, uv1(0,1,0), uv2(0,1,0);
    if (u1) uv1 = parseVec3fList(u1);
    if (u2) uv2 = parseVec3fList(u2);
    self->objPtr->addPoint(parseVec3fList(p1), parseVec3fList(n1), c, uv1);
    self->objPtr->addPoint(parseVec3fList(p2), parseVec3fList(n2), c, uv2);
    self->objPtr->compute(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::close(VRPyPath* self) {
	if (!self->valid()) return NULL;
    self->objPtr->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::invert(VRPyPath* self) {
	if (!self->valid()) return NULL;
    self->objPtr->invert();
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::addPoint(VRPyPath* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject *_p, *_n, *_c, *_u; _p=_n=_c=_u=0;
    if (! PyArg_ParseTuple(args, "|OOOO", &_p, &_n, &_c, &_u)) return NULL;

    OSG::Vec3f p, n, c, u;
    p = _p ? parseVec3fList(_p) : OSG::Vec3f(0,0,0);
    n = _n ? parseVec3fList(_n) : OSG::Vec3f(0,0,-1);
    c = _c ? parseVec3fList(_c) : OSG::Vec3f(0,0,0);
    u = _u ? parseVec3fList(_u) : OSG::Vec3f(0,1,0);

    self->objPtr->addPoint(p,n,c,u);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::setStartPoint(VRPyPath* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = parseVec3fList(_p);
    n = parseVec3fList(_n);
    c = parseVec3fList(_c);

    self->objPtr->addPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::setEndPoint(VRPyPath* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = parseVec3fList(_p);
    n = parseVec3fList(_n);
    c = parseVec3fList(_c);

    self->objPtr->addPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::getStartPoint(VRPyPath* self) {
	if (!self->valid()) return NULL;
    OSG::Vec3f v[3];
    auto pnt = self->objPtr->getPoint(0);
    v[0] = pnt.p;
    v[1] = pnt.n;
    v[2] = pnt.c;

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::getEndPoint(VRPyPath* self) {
	if (!self->valid()) return NULL;
    OSG::Vec3f v[3];
    auto pnt = self->objPtr->getPoint(1);
    v[0] = pnt.p;
    v[1] = pnt.n;
    v[2] = pnt.c;

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::compute(VRPyPath* self, PyObject* args) {
	if (!self->valid()) return NULL;
    int N; N=0;
    if (! PyArg_ParseTuple(args, "i", &N)) return NULL;

    self->objPtr->compute(N);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::update(VRPyPath* self) {
	if (!self->valid()) return NULL;
    self->objPtr->update();
    Py_RETURN_TRUE;
}



