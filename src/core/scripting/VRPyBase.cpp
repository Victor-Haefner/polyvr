#include "VRPyBase.h"


PyObject* VRPyBase::err = NULL;

PyObject* VRPyBase::parseObject(PyObject *args) {
    PyObject* o = NULL;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    return o;
}

vector<PyObject*> VRPyBase::pyListToVector(PyObject *v) {
    std::string type = v->ob_type->tp_name;
    if (type == "list") v = PyList_AsTuple(v);

    cout << " t " << type;

    PyObject *pi;
    int N = PyTuple_Size(v);
    vector<PyObject*> res;
    for (int i=0; i<N; i++) {
        pi = PyTuple_GetItem(v, i);
        res.push_back(pi);
    }

    cout << " R " << N << endl;
    return res;
}

vector<PyObject*> VRPyBase::parseList(PyObject *args) {
    return pyListToVector( parseObject(args) );
}

OSG::Vec3f VRPyBase::parseVec3fList(PyObject *li) {
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
    int aL = PyTuple_Size(args);
    if (aL == 1) return parseVec3fList( parseObject(args) );

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
