#include "VRPyBase.h"
#include "VRPyBaseT.h"
#include "VRPyMath.h"

#include <OpenSG/OSGImage.h>


PyObject* VRPyBase::err = NULL;

PyObject* VRPyBase::parseObject(PyObject *args) {
    PyObject* o = NULL;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    return o;
}

PyObject* VRPyBase::setErr(string e) {
    PyErr_SetString(err, e.c_str());
    return NULL;
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
    if (VRPyVec2f::check(v)) return 2;
    if (VRPyVec3f::check(v)) return 3;
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

OSG::Vec2i VRPyBase::parseVec2iList(PyObject *li) {
    if (li == 0) return OSG::Vec2i();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 2) return OSG::Vec2i();
    int x,y;
    x = PyInt_AsLong(lis[0]);
    y = PyInt_AsLong(lis[1]);
    return OSG::Vec2i(x,y);
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

OSG::Vec4i VRPyBase::parseVec4iList(PyObject *li) {
    if (li == 0) return OSG::Vec4i();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 4) return OSG::Vec4i();
    int x,y,z,w;
    x = PyInt_AsLong(lis[0]);
    y = PyInt_AsLong(lis[1]);
    z = PyInt_AsLong(lis[2]);
    w = PyInt_AsLong(lis[3]);
    return OSG::Vec4i(x,y,z,w);
}

OSG::Line VRPyBase::PyToLine(PyObject *li) {
    if (li == 0) return OSG::Line();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 6) return OSG::Line();
    float r[6];
    for (int i=0; i<6; i++) r[i] = PyFloat_AsDouble(lis[i]);
    return OSG::Line(OSG::Pnt3f(r[3],r[4],r[5]), OSG::Vec3f(r[0],r[1],r[2]));
}

OSG::Vec2d VRPyBase::parseVec2dList(PyObject *li) {
    if (li == 0) return OSG::Vec2d();
    if (VRPyVec2f::check(li)) return ((VRPyVec2f*)li)->v;
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 2) return OSG::Vec2d();
    float x,y;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    return OSG::Vec2d(x,y);
}

OSG::Vec3d VRPyBase::parseVec3dList(PyObject *li) {
    if (li == 0) return OSG::Vec3d();
    if (VRPyVec3f::check(li)) return ((VRPyVec3f*)li)->v;
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 3) return OSG::Vec3d();
    float x,y,z;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    z = PyFloat_AsDouble(lis[2]);
    return OSG::Vec3d(x,y,z);
}

OSG::Vec4d VRPyBase::parseVec4dList(PyObject *li) {
    if (li == 0) return OSG::Vec4d();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 4) return OSG::Vec4d();
    float x,y,z,w;
    x = PyFloat_AsDouble(lis[0]);
    y = PyFloat_AsDouble(lis[1]);
    z = PyFloat_AsDouble(lis[2]);
    w = PyFloat_AsDouble(lis[3]);
    return OSG::Vec4d(x,y,z,w);
}

OSG::Matrix4d VRPyBase::parseMatrixList(PyObject *li) {
    if (li == 0) return OSG::Matrix4d();
    vector<PyObject*> lis = pyListToVector(li);
    if (lis.size() != 16) return OSG::Matrix4d();
    float d[16];
    for (int i=0; i<16; i++) d[i] = PyFloat_AsDouble(lis[i]);
    return OSG::Matrix4d(d[0], d[1], d[2], d[3], d[4], d[5],d[6],d[7],d[8], d[9],d[10], d[11], d[12], d[13],d[14], d[15]);
}

OSG::Vec2d VRPyBase::parseVec2f(PyObject *args) {
    if (pySize(args) == 1) return parseVec2dList( parseObject(args) );

    float x,y; x=y=0;
    if (! PyArg_ParseTuple(args, "ff", &x, &y)) return OSG::Vec2d();
    return OSG::Vec2d(x,y);
}

OSG::Vec3d VRPyBase::parseVec3d(PyObject *args) {
    if (pySize(args) == 1) return parseVec3dList( parseObject(args) );

    float x,y,z; x=y=z=0;
    if (! PyArg_ParseTuple(args, "fff", &x, &y, &z)) return OSG::Vec3d();
    return OSG::Vec3d(x,y,z);
}

OSG::Vec4d VRPyBase::parseVec4d(PyObject *args) {
    if (pySize(args) == 1) return parseVec4dList( parseObject(args) );

    float x,y,z,w; x=y=z=w=0;
    if (! PyArg_ParseTuple(args, "ffff", &x, &y, &z, &w)) return OSG::Vec4d();
    return OSG::Vec4d(x,y,z,w);
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

PyObject* VRPyBase::toPyTuple(const OSG::Vec2d& v) {
    PyObject* res = PyList_New(2);
    for (int i=0; i<2; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(const OSG::Vec3d& v) {
    PyObject* res = PyList_New(3);
    for (int i=0; i<3; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(const OSG::Vec4d& v) {
    PyObject* res = PyList_New(4);
    for (int i=0; i<4; i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(const OSG::Vec2i& v) {
    PyObject* res = PyList_New(2);
    for (int i=0; i<2; i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(const OSG::Vec3i& v) {
    PyObject* res = PyList_New(3);
    for (int i=0; i<3; i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple(const OSG::Vec4i& v) {
    PyObject* res = PyList_New(4);
    for (int i=0; i<4; i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
    return res;
}

PyObject* VRPyBase::toPyTuple( const vector<string>& v ) {
    PyObject* res = PyList_New(v.size());
    for (uint i=0; i<v.size(); i++) PyList_SetItem(res, i, PyString_FromString(v[i].c_str()));
    return res;
}

PyObject* VRPyBase::toPyTuple( const vector<PyObject*>& v ) {
    PyObject* res = PyList_New(v.size());
    for (uint i=0; i<v.size(); i++) PyList_SetItem(res, i, v[i]);
    return res;
}

int VRPyBase::toOSGConst(PyObject* o) { return toOSGConst( PyString_AsString(o) ); }
int VRPyBase::toGLConst(PyObject* o) { return toGLConst( PyString_AsString(o) ); }

int VRPyBase::toOSGConst(string s) {
    // pixel formats
    if (s == "A") return OSG::Image::OSG_A_PF;
    if (s == "I") return OSG::Image::OSG_I_PF;
    if (s == "L") return OSG::Image::OSG_L_PF;
    if (s == "LA") return OSG::Image::OSG_LA_PF;
    if (s == "RGB") return OSG::Image::OSG_RGB_PF;
    if (s == "RGBA") return OSG::Image::OSG_RGBA_PF;
    if (s == "BGR") return OSG::Image::OSG_BGR_PF;
    if (s == "BGRA") return OSG::Image::OSG_BGRA_PF;
    if (s == "RGB_DXT1") return OSG::Image::OSG_RGB_DXT1;
    if (s == "RGBA_DXT1") return OSG::Image::OSG_RGBA_DXT1;
    if (s == "RGBA_DXT3") return OSG::Image::OSG_RGBA_DXT3;
    if (s == "RGBA_DXT5") return OSG::Image::OSG_RGBA_DXT5;
    if (s == "DEPTH") return OSG::Image::OSG_DEPTH_PF;
    if (s == "DEPTH_STENCIL") return OSG::Image::OSG_DEPTH_STENCIL_PF;

    if (s == "A_FLT") return GL_ALPHA32F_ARB;
    if (s == "L_FLT") return GL_LUMINANCE32F_ARB;
    if (s == "LA_FLT") return GL_LUMINANCE_ALPHA32F_ARB;
    if (s == "RGB_FLT") return GL_RGB32F;
    if (s == "RGBA_FLT") return GL_RGBA32F;

    if (s == "A_INT") return OSG::Image::OSG_ALPHA_INTEGER_PF;
    if (s == "L_INT") return OSG::Image::OSG_LUMINANCE_INTEGER_PF;
    if (s == "LA_INT") return OSG::Image::OSG_LUMINANCE_ALPHA_INTEGER_PF;
    if (s == "RGB_INT") return OSG::Image::OSG_RGB_INTEGER_PF;
    if (s == "RGBA_INT") return OSG::Image::OSG_RGBA_INTEGER_PF;
    if (s == "BGR_INT") return OSG::Image::OSG_BGR_INTEGER_PF;
    if (s == "BGRA_INT") return OSG::Image::OSG_BGRA_INTEGER_PF;

    // image data types
    if (s == "UINT8") return OSG::Image::OSG_UINT8_IMAGEDATA;
    if (s == "UINT16") return OSG::Image::OSG_UINT16_IMAGEDATA;
    if (s == "UINT32") return OSG::Image::OSG_UINT32_IMAGEDATA;
    if (s == "FLOAT16") return OSG::Image::OSG_FLOAT16_IMAGEDATA;
    if (s == "FLOAT32") return OSG::Image::OSG_FLOAT32_IMAGEDATA;
    if (s == "INT16") return OSG::Image::OSG_INT16_IMAGEDATA;
    if (s == "INT32") return OSG::Image::OSG_INT32_IMAGEDATA;
    if (s == "UINT24_8") return OSG::Image::OSG_UINT24_8_IMAGEDATA;

    return -1;
}

int VRPyBase::toGLConst(string s) {
    if (s == "GL_FILL") return GL_FILL;
    if (s == "GL_BACK") return GL_BACK;
    if (s == "GL_NONE") return GL_NONE;
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
    if (s == "GL_PATCHES") return GL_PATCHES;
    if (s == "GL_TRIANGLE_STRIP") return GL_TRIANGLE_STRIP;
    if (s == "GL_TRIANGLE_FAN") return GL_TRIANGLE_FAN;
    if (s == "GL_LINE_STRIP") return GL_LINE_STRIP;
    if (s == "GL_LINE_LOOP") return GL_LINE_LOOP;
    if (s == "GL_QUAD_STRIP") return GL_QUAD_STRIP;
    if (s == "GL_POLYGON") return GL_POLYGON;

    if (s == "GL_REPEAT") return GL_REPEAT;
    if (s == "GL_CLAMP") return GL_CLAMP;
    if (s == "GL_CLAMP_TO_EDGE") return GL_CLAMP_TO_EDGE;
    if (s == "GL_CLAMP_TO_BORDER") return GL_CLAMP_TO_BORDER;

    if (s == "GL_NEAREST") return GL_NEAREST;
    if (s == "GL_LINEAR") return GL_LINEAR;
    if (s == "GL_NEAREST") return GL_NEAREST;
    if (s == "GL_LINEAR") return GL_LINEAR;
    if (s == "GL_NEAREST_MIPMAP_NEAREST") return GL_NEAREST_MIPMAP_NEAREST;
    if (s == "GL_LINEAR_MIPMAP_NEAREST") return GL_LINEAR_MIPMAP_NEAREST;
    if (s == "GL_NEAREST_MIPMAP_LINEAR") return GL_NEAREST_MIPMAP_LINEAR;
    if (s == "GL_LINEAR_MIPMAP_LINEAR") return GL_LINEAR_MIPMAP_LINEAR;

    return -1;
}

bool VRPyBase::isNone(PyObject* o) { return (o == Py_None); }

void VRPyBase::execPyCallVoid(PyObject* pyFkt, PyObject* pArgs) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();

    PyObject_CallObject(pyFkt, pArgs);

    //Py_XDECREF(pArgs); Py_DecRef(pyFkt); // TODO!!

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}
