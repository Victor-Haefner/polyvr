#include "VRPyBase.h"


PyObject* VRPyBase::err = NULL;

PyObject* VRPyBase::parseObject(PyObject *args) {
    PyObject* o = NULL;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    return o;
}

vector<PyObject*> VRPyBase::pyListToVector(PyObject *v) {
    vector<PyObject*> res;
    if(isList(v))  for (int i=0; i<pySize(v); i++) res.push_back(PyList_GetItem(v, i));
    if(isTuple(v)) for (int i=0; i<pySize(v); i++) res.push_back(PyTuple_GetItem(v, i));
    return res;
}

vector<PyObject*> VRPyBase::parseList(PyObject *args) {
    return pyListToVector( parseObject(args) );
}

bool VRPyBase::isList(PyObject* o) { return PyList_Check(o); }
bool VRPyBase::isTuple(PyObject* o) { return PyTuple_Check(o); }

int VRPyBase::pySize(PyObject* v) {
    if (isList(v)) return PyList_Size(v);
    if (isTuple(v)) return PyTuple_Size(v);
    return 0;
}

PyObject* VRPyBase::getItem(PyObject* v, int i) {
    if (i < 0 || i >= pySize(v)) return 0;
    if (isList(v)) return PyList_GetItem(v,i);
    if (isTuple(v)) return PyTuple_GetItem(v,i);
    return 0;
}

OSG::Vec3i VRPyBase::parseVec3iList(PyObject *li) {
    if (li == 0) return OSG::Vec3i();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 3) return OSG::Vec3i();
    int x,y,z;
    x = PyInt_AsLong(lis[0]);
    y = PyInt_AsLong(lis[1]);
    z = PyInt_AsLong(lis[2]);
    return OSG::Vec3i(x,y,z);
}

OSG::Line VRPyBase::PyToLine(PyObject *li) {
    if (li == 0) return OSG::Line();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 6) return OSG::Line();
    float r[6];
    for (int i=0; i<6; i++) r[i] = PyFloat_AsDouble(lis[i]);
    return OSG::Line(OSG::Pnt3f(r[3],r[4],r[5]), OSG::Vec3f(r[0],r[1],r[2]));
}

OSG::Vec2f VRPyBase::parseVec2fList(PyObject *li) {
    if (li == 0) return OSG::Vec2f();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 2) return OSG::Vec2f();
    float x,y;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    return OSG::Vec2f(x,y);
}

OSG::Vec3f VRPyBase::parseVec3fList(PyObject *li) {
    if (li == 0) return OSG::Vec3f();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 3) return OSG::Vec3f();
    float x,y,z;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    z = PyFloat_AsDouble(lis[2]);
    return OSG::Vec3f(x,y,z);
}

OSG::Vec4f VRPyBase::parseVec4fList(PyObject *li) {
    if (li == 0) return OSG::Vec4f();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 4) return OSG::Vec4f();
    float x,y,z,w;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    z = PyFloat_AsDouble(lis[2]);
    w = PyFloat_AsDouble(lis[3]);
    return OSG::Vec4f(x,y,z,w);
}

OSG::Vec2f VRPyBase::parseVec2f(PyObject *args) {
    float x,y; x=y=0;
    if (! PyArg_ParseTuple(args, "ff", &x, &y)) return OSG::Vec2f();
    return OSG::Vec2f(x,y);
}

OSG::Vec3f VRPyBase::parseVec3f(PyObject *args) {
    if (pySize(args) == 1) return parseVec3fList( parseObject(args) );

    float x,y,z; x=y=z=0;
    if (! PyArg_ParseTuple(args, "fff", &x, &y, &z)) return OSG::Vec3f();
    return OSG::Vec3f(x,y,z);
}

OSG::Vec4f VRPyBase::parseVec4f(PyObject *args) {
    float x,y,z,w; x=y=z=w=0;
    if (! PyArg_ParseTuple(args, "ffff", &x, &y, &z, &w)) return OSG::Vec4f();
    return OSG::Vec4f(x,y,z,w);
}

OSG::Vec3i VRPyBase::parseVec3i(PyObject *args) {
    if (pySize(args) == 1) return parseVec3iList( parseObject(args) );

    int x,y,z; x=y=z=0;
    if (! PyArg_ParseTuple(args, "iii", &x, &y, &z)) return OSG::Vec3i();
    return OSG::Vec3i(x,y,z);
}

float VRPyBase::parseFloat(PyObject *args) {
    float x; x=0;
    if (! PyArg_ParseTuple(args, "f", &x)) return 0;
    return x;
}

bool VRPyBase::parseBool(PyObject *args) {
    int x; x=0;
    if (! PyArg_ParseTuple(args, "i", &x)) return 0;
    return x;
}

int VRPyBase::parseInt(PyObject *args) {
    int x; x=0;
    if (! PyArg_ParseTuple(args, "i", &x)) return 0;
    return x;
}

string VRPyBase::parseString(PyObject *args) {
    PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return "";
    return PyString_AsString(o);
}

PyObject* VRPyBase::toPyTuple(OSG::Vec3f v) {
    PyObject* res = PyList_New(3);
    for (int i=0; i<3; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}
PyObject* VRPyBase::toPyTuple(OSG::Vec3i v) {
    PyObject* res = PyList_New(3);
    for (int i=0; i<3; i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(OSG::Vec2f v) {
    PyObject* res = PyList_New(2);
    for (int i=0; i<2; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

int VRPyBase::toGLConst(PyObject* o) {
    string s = PyString_AsString(o);
    return toGLConst(s);
}

int VRPyBase::toGLConst(string s) {
    if (s == "GL_NEVER") return GL_NEVER;
    if (s == "GL_ALWAYS") return GL_ALWAYS;
    if (s == "GL_EQUAL") return GL_EQUAL;
    if (s == "GL_NOTEQUAL") return GL_NOTEQUAL;
    if (s == "GL_LESS") return GL_LESS;
    if (s == "GL_LEQUAL") return GL_LEQUAL;
    if (s == "GL_GEQUAL") return GL_GEQUAL;
    if (s == "GL_GREATER") return GL_GREATER;
    if (s == "GL_KEEP") return GL_KEEP;
    if (s == "GL_ZERO") return GL_ZERO;
    if (s == "GL_REPLACE") return GL_REPLACE;
	if (s == "GL_INCR") return GL_INCR;
	if (s == "GL_DECR") return GL_DECR;
#ifdef GL_INCR_WRAP
    if (s == "GL_INCR_WRAP") return GL_INCR_WRAP;
#endif
#ifdef GL_DECR_WRAP
	if (s == "GL_DECR_WRAP") return GL_DECR_WRAP;
#endif
    if (s == "GL_INVERT") return GL_INVERT;

    if (s == "GL_QUADS") return GL_QUADS;
    if (s == "GL_TRIANGLES") return GL_TRIANGLES;
    if (s == "GL_LINES") return GL_LINES;
    if (s == "GL_POINTS") return GL_POINTS;
    if (s == "GL_TRIANGLE_STRIP") return GL_TRIANGLE_STRIP;
    if (s == "GL_TRIANGLE_FAN") return GL_TRIANGLE_FAN;
    if (s == "GL_LINE_STRIP") return GL_LINE_STRIP;
    if (s == "GL_LINE_LOOP") return GL_LINE_LOOP;
    if (s == "GL_QUAD_STRIP") return GL_QUAD_STRIP;
    if (s == "GL_POLYGON") return GL_POLYGON;

    return -1;
}

bool VRPyBase::isNone(PyObject* o) { return (o == Py_None); }

template<class T>
PyTypeObject VRPyBaseT<T>::type = NULL;
