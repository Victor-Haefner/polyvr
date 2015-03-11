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
    if (lis.size() != 3) return OSG::Vec3i(0,0,0);
    int x,y,z;
    x = PyInt_AsLong(lis[0]);
    y = PyInt_AsLong(lis[1]);
    z = PyInt_AsLong(lis[2]);
    return OSG::Vec3i(x,y,z);
}

OSG::Vec3f VRPyBase::parseVec3fList(PyObject *li) {
    if (li == 0) return OSG::Vec3f();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 3) return OSG::Vec3f(0,0,0);
    float x,y,z;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    z = PyFloat_AsDouble(lis[2]);
    return OSG::Vec3f(x,y,z);
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

PyObject* VRPyBase::toPyTuple(OSG::Vec2f v) {
    PyObject* res = PyList_New(2);
    for (int i=0; i<2; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

template<class T>
PyTypeObject VRPyBaseT<T>::type = NULL;
