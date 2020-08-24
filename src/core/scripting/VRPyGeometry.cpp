#include "VRPyGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRVideo.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyMaterial.h"
#include "VRPySelection.h"
#include "VRPyTypeCaster.h"
#include "VRPyPose.h"
#include "VRPyBoundingbox.h"
#include "VRPyMath.h"

#ifndef WITHOUT_NUMPY
#define NO_IMPORT_ARRAY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarraytypes.h>
#include <numpy/ndarrayobject.h>
#endif

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

simpleVRPyType( Geometry, New_VRObjects_ptr );

PyMethodDef VRPyGeometry::methods[] = {
    {"setType", PyWrap( Geometry, setType, "set geometry type", void, int ) },
    {"setTypes", (PyCFunction)VRPyGeometry::setTypes, METH_VARARGS, "set geometry type - setTypes([type1, type2, ..])\n\ttype can be:"
                                                                                                                    "\n\t GL_POINTS"
                                                                                                                    "\n\t GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP"
                                                                                                                    "\n\t GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN"
                                                                                                                    "\n\t GL_QUADS, GL_QUAD_STRIP"
                                                                                                                    "\n\t GL_POLYGON" },
    {"setPositions", (PyCFunction)VRPyGeometry::setPositions, METH_VARARGS, "set geometry positions - setPositions(list of (list of [x,y,z]))" },
    {"setNormals", (PyCFunction)VRPyGeometry::setNormals, METH_VARARGS, "set geometry normals - setNormals([[x,y,z], ...])" },
    {"setColors", (PyCFunction)VRPyGeometry::setColors, METH_VARARGS, "set geometry colors - setColors([[x,y,z], ...])" },
    {"setIndices", (PyCFunction)VRPyGeometry::setIndices, METH_VARARGS, "set geometry indices - setIndices(int[])" },
    {"setLengths", (PyCFunction)VRPyGeometry::setLengths, METH_VARARGS, "set geometry lengths - setLengths(int[])" },
    {"setTexCoords", (PyCFunction)VRPyGeometry::setTexCoords, METH_VARARGS, "set geometry texture coordinates - setTexCoords( [[x,y]], int channel = 0, bool fixMapping = false)" },
    {"setColor", PyWrap(Geometry, setColor, "Set a colored material to the geometry", void, string) },
    {"setMaterial", PyWrap(Geometry, setMaterial, "Set the material of the geometry", void, VRMaterialPtr) },
    {"flipNormals", PyWrap(Geometry, flipNormals, "Flip normals", void) },
    {"getTypes", (PyCFunction)VRPyGeometry::getTypes, METH_NOARGS, "get geometry primitive types - [int t] getTypes()\n\tt = 0 : GL_POINTS"
                                                                                                                    "\n\tt = 1 : GL_LINES"
                                                                                                                    "\n\tt = 2 : GL_LINE_LOOP"
                                                                                                                    "\n\tt = 3 : GL_LINE_STRIP"
                                                                                                                    "\n\tt = 4 : GL_TRIANGLES"
                                                                                                                    "\n\tt = 5 : GL_TRIANGLE_STRIP"
                                                                                                                    "\n\tt = 6 : GL_TRIANGLE_FAN"
                                                                                                                    "\n\tt = 7 : GL_QUADS"
                                                                                                                    "\n\tt = 8 : GL_QUAD_STRIP"
                                                                                                                    "\n\tt = 9 : GL_POLYGON" },
    {"getLengths", (PyCFunction)VRPyGeometry::getLengths, METH_NOARGS, "get geometry lengths" },
    {"getPositions", (PyCFunction)VRPyGeometry::getPositions, METH_NOARGS, "get geometry positions" },
    {"getNormals", (PyCFunction)VRPyGeometry::getNormals, METH_NOARGS, "get geometry normals" },
    {"getColors", (PyCFunction)VRPyGeometry::getColors, METH_NOARGS, "get geometry colors" },
    {"getIndices", (PyCFunction)VRPyGeometry::getIndices, METH_NOARGS, "get geometry indices" },
    {"getTexCoords", (PyCFunction)VRPyGeometry::getTexCoords, METH_VARARGS, "get geometry texture coordinates" },
    {"getMaterial", PyWrap( Geometry, getMaterial, "get material", VRMaterialPtr ) },
    {"getGeometricCenter", PyWrap(Geometry, getGeometricCenter, "Get geometric center", Vec3d ) },
    {"merge", PyWrapOpt( Geometry, merge, "Merge another geometry into this one - merge( geo )", "0", void, VRGeometryPtr, PosePtr ) },
    {"remove", PyWrap( Geometry, removeSelection, "Remove a part of the geometry - remove( Selection s )", void, VRSelectionPtr ) },
    {"copy", PyWrap( Geometry, copySelection, "Copy a part of the geometry - geo copy( Selection s )", VRGeometryPtr, VRSelectionPtr ) },
    {"separate", PyWrap( Geometry, separateSelection, "Copy and remove a part of the geometry - geo separate( Selection s )", VRGeometryPtr, VRSelectionPtr ) },
    {"setPrimitive", PyWrap( Geometry, setPrimitive, "Set geometry to primitive - setPrimitive(str params)"
                        "\n\tparams is a single string as for example: 'Box 1.2 5.4 3.46 1 12 32'"
                        "\n\t   Box is the primitive type, followed by the geometric parameters"
                        "\n\tavailable primitives are:"
                        "\n\t\tPlane size_x size_y segments_x segments_y"
                        "\n\t\tBox size_x size_y size_z segments_x segments_y segments_z"
                        "\n\t\tSphere radius iterations"
                        "\n\t\tCylinder height radius N_sides do_bottom do_top do_sides"
                        "\n\t\tCone height radius N_sides do_bottom do_sides"
                        "\n\t\tTorus inner_radius outer_radius N_segments N_rings"
                        "\n\t\tTeapot iterations scale"
                        "\n\t\tArrow height width trunc hat thickness"
                        "\n\t\tGear width hole pitch N_teeth teeth_size bevel"
                        "\n\t\tThread length radius pitch N_segments", void, string ) },
    {"decimate", PyWrap( Geometry, decimate, "Decimate geometry by collapsing a fraction of edges - decimate(f)", void, float ) },
    {"setRandomColors", PyWrap( Geometry, setRandomColors, "Set a random color for each vertex", void ) },
    {"removeDoubles", PyWrap( Geometry, removeDoubles, "Remove double vertices", void, float ) },
    {"splitByVertexColors", PyWrap( Geometry, splitByVertexColors, "Split geometry ba vertex colors", vector<VRGeometryPtr> ) },
    {"updateNormals", PyWrapOpt( Geometry, updateNormals, "Recalculate the normals of the geometry - updateNormals(| bool face)\n\tset face to true to compute face normals, the default are vertex normals", "0", void, bool ) },
    {"makeUnique", PyWrap( Geometry, makeUnique, "Make the geometry data unique", void ) },
    {"influence", PyWrapOpt( Geometry, influence, "Pass a points and value vector to influence the geometry - influence([points,f3], [values,f3], int power)", "-1|1", void, vector<Vec3d>, vector<Vec3d>, int, float, float ) },
    {"showGeometricData", PyWrap( Geometry, showGeometricData, "Enable or disable a data layer - showGeometricData(string type, bool)\n layers are: ['Normals']", void, string, bool ) },
    {"calcSurfaceArea", PyWrap( Geometry, calcSurfaceArea, "Compute and return the total surface area - flt calcSurfaceArea()", float ) },
    {"setPositionalTexCoords", PyWrap( Geometry, setPositionalTexCoords, "Use the positions as texture coordinates - setPositionalTexCoords(float scale, int texID, [i,j,k] format)", void, float, int, Vec3i ) },
    {"setPositionalTexCoords2D", PyWrap( Geometry, setPositionalTexCoords2D, "Use the positions as texture coordinates - setPositionalTexCoords2D(float scale, int texID, [i,j] format)", void,float, int, Vec2i ) },
    {"genTexCoords", PyWrap( Geometry, genTexCoords, "Generate the texture coordinates - genTexCoords( str mapping, float scale, int channel, Pose )\n\tmapping: ['CUBE', 'SPHERE']", void, string, float, int, PosePtr ) },
    {"readSharedMemory", PyWrap( Geometry, readSharedMemory, "Read the geometry from shared memory buffers - readSharedMemory( str segment, str object )", void, string, string ) },
    {"setPatchVertices", PyWrap(Geometry, setPatchVertices, "Set patch primitives for tesselation shader", void, int) },
    {"setMeshVisibility", PyWrap(Geometry, setMeshVisibility, "Set mesh visibility", void, bool) },
    {"convertToTriangles", PyWrap(Geometry, convertToTriangles, "Convert geometry to triangles", void) },
    {"convertToTrianglePatches", PyWrap(Geometry, convertToTrianglePatches, "Convert to triangles patches, necessary for displacement maps", void) },
    {"convertToPointCloud", PyWrap(Geometry, convertToPointCloud, "Convert geometry to pointcloud - convertToPointCloud(options = None)"
                        "\n\t\topts = {}"
                        "\n\t\topts['lit'] = 0"
                        "\n\t\topts['resolution'] = 1"
                        "\n\t\topts['pointSize'] = 5"
                        "\n\t\topts['lod1'] = [5, 20]"
                        "\n\t\topts['lod2'] = [10, 200]"
                        "\n\t\topts['swapYZ'] = 1"
                        "\n\t\topts['keepOctree'] = 1", VRPointCloudPtr, map<string, string>)},

    {"addVertex", (PyCFunction)VRPyGeometry::addVertex, METH_VARARGS, "Add a vertex to geometry - addVertex( pos | norm, col, tc )" },
    {"setVertex", (PyCFunction)VRPyGeometry::setVertex, METH_VARARGS, "Set a vertex - setVertex( int i, pos | norm, col, tc )" },
    {"addPoint", PyWrapOpt( Geometry, addPoint, "Add a point to geometry - addPoint( | int i )", "-1", void, int ) },
    {"addLine", PyWrapOpt( Geometry, addLine, "Add a line to geometry - addLine( | [i1,i2] )", "-2 -1", void, Vec2i ) },
    {"addTriangle", PyWrapOpt( Geometry, addTriangle, "Add a triangle to geometry - addTriangle( | [i1,i2,i3] )", "-3 -2 -1", void, Vec3i ) },
    {"addQuad", PyWrapOpt( Geometry, addQuad, "Add a quad to geometry - addQuad( | [i1,i2,i3,i4] )", "-4 -3 -2 -1", void, Vec4i ) },
    {"clear", PyWrap( Geometry, clear, "Clear all geometric data - clear()", void ) },
    {"size", PyWrap( Geometry, size, "Returns the size of the positions vector", int ) },
    {"remColors", PyWrapOpt( Geometry, remColors, "Removes color data", "0", void, bool ) },
    {"makeSingleIndex", PyWrap( Geometry, makeSingleIndex, "Make geometry single index", void ) },
    {NULL}  /* Sentinel */
};

/*
Positions get/set, setTypes
*/

int getListDepth(PyObject* o) {
    string tname;
	tname = o->ob_type->tp_name;
	if (tname == "list") if (PyList_Size(o) > 0) return getListDepth(PyList_GetItem(o, 0))+1;
#ifndef WITHOUT_NUMPY
	if (tname == "numpy.ndarray") {
        PyArrayObject* a = (PyArrayObject*)o;
        return PyArray_NDIM(a);
	}
#endif
	if (tname == "VR.Math.Vec3") return 1;
	return 0;
}

#ifndef WITHOUT_NUMPY
template<class T, class t>
void feed2Dnp(PyObject* o, T& vec) { // numpy version
    PyArrayObject* a = (PyArrayObject*)o;

    int N = PyArray_DIMS(a)[0];
    vec->resize(N);

    t v;
    double* f;
    for (int i=0; i<N; i++) {
        for (int j=0; j<3; j++) {
            f = (double*)PyArray_GETPTR2(a, i, j);
            v[j] = *f;
        }
        vec->setValue(v, i);
    }
}
#endif

template<class T, class t>
void feed2D(PyObject* o, T& vec) {
    t tmp;
    for (Py_ssize_t i=0; i<PyList_Size(o); i++) {
        toValue(PyList_GetItem(o,i), tmp);
        vec->push_back(tmp);
    }
}

template<class T, class t>
void feed2D_v2(PyObject* o, T& vec) {
    t tmp;
    for (Py_ssize_t i=0; i<PyList_Size(o); i++) {
        toValue(PyList_GetItem(o,i), tmp);
        vec.push_back(tmp);
    }
}

#ifndef WITHOUT_NUMPY
template<class T>
void feed1Dnp(PyObject* o, T& vec) {
    PyArrayObject* a = (PyArrayObject*)o;
    int N = PyArray_DIMS(a)[0];
    vec->resize(N);

    for (Py_ssize_t i=0; i<N; i++) {
        int* j = (int*)PyArray_GETPTR1(a, i);
        vec->setValue(*j, i);
    }
}
#endif

template<class T>
void feed1D(PyObject* o, T& vec) {
    PyObject *pi;
    Py_ssize_t N = PyList_Size(o);

    for (Py_ssize_t i=0; i<N; i++) {
        pi = PyList_GetItem(o, i);
        int j = PyInt_AsLong(pi);
        vec->addValue(j);
    }
}

#ifndef WITHOUT_NUMPY
template<class T, class t>
void feed1D3np(PyObject* o, T& vec) {
    PyArrayObject* a = (PyArrayObject*)o;
    int N = PyArray_DIMS(a)[0];
    for (Py_ssize_t i=0; i<N; i+=3) {
        t tmp;
        float* x = (float*)PyArray_GETPTR1(a, i+0);
        float* y = (float*)PyArray_GETPTR1(a, i+1);
        float* z = (float*)PyArray_GETPTR1(a, i+2);
        tmp[0] = *x;
        tmp[1] = *y;
        tmp[2] = *z;
        vec->addValue(tmp);
    }
}
#endif

template<class T, class t>
void feed1D3(PyObject* o, T& vec) {
    Py_ssize_t N = PyList_Size(o);

    for (Py_ssize_t i=0; i<N; i+=3) {
        t tmp;
        tmp[0] = PyFloat_AsDouble( PyList_GetItem(o, i+0) );
        tmp[1] = PyFloat_AsDouble( PyList_GetItem(o, i+1) );
        tmp[2] = PyFloat_AsDouble( PyList_GetItem(o, i+2) );
        vec->addValue(tmp);
    }
}

PyObject* VRPyGeometry::fromSharedPtr(VRGeometryPtr obj) {
    return VRPyTypeCaster::cast(dynamic_pointer_cast<VRObject>(obj));
}

PyObject* VRPyGeometry::addVertex(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject *p, *n, *c, *t;
    p = n = c = t = 0;
    if (!PyArg_ParseTuple(args, "O|OOO", &p, &n, &c, &t)) return NULL;
    VRGeoData geo(self->objPtr);

    bool doTC = (t != 0) && !isNone(t);
    bool doN = (n != 0) && !isNone(n);
    bool doC = (c != 0) && !isNone(c);

    int res = -1;
    if (doN && doC && doTC) res = geo.pushVert(parseVec3dList(p), parseVec3dList(n), Vec3f(parseVec3dList(c)), parseVec2dList(t));
    else if (doN && doC) res = geo.pushVert(parseVec3dList(p), parseVec3dList(n), Vec3f(parseVec3dList(c)));
    else if (doN && doTC) res = geo.pushVert(parseVec3dList(p), parseVec3dList(n), parseVec2dList(t));
    else if (doN) res = geo.pushVert(parseVec3dList(p), parseVec3dList(n));
    else res = geo.pushVert(parseVec3dList(p));

    if (res == 0) geo.apply(self->objPtr, false);
    return PyInt_FromLong(res);
}

PyObject* VRPyGeometry::setVertex(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int i;
    PyObject *p, *n, *c, *t;
    p = n = c = t = 0;
    if (!PyArg_ParseTuple(args, "iO|OOO", &i, &p, &n, &c, &t)) return NULL;
    VRGeoData geo(self->objPtr);

    bool doTC = (t != 0) && !isNone(t);
    bool doN = (n != 0) && !isNone(n);
    bool doC = (c != 0) && !isNone(c);
    if (doN && doC && doTC) geo.setVert(i, parseVec3dList(p), parseVec3dList(n), Vec3f(parseVec3dList(c)), parseVec2dList(t));
    else if (doN && doC) geo.setVert(i, parseVec3dList(p), parseVec3dList(n), Vec3f(parseVec3dList(c)));
    else if (doN && doTC) geo.setVert(i, parseVec3dList(p), parseVec3dList(n), parseVec2dList(t));
    else if (doN) geo.setVert(i, parseVec3dList(p), parseVec3dList(n));
    else geo.setVert(i, parseVec3dList(p));
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setTypes(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
	PyObject* typeList;
    if (!PyArg_ParseTuple(args, "O", &typeList)) return NULL;

    VRGeometryPtr geo = (VRGeometryPtr) self->objPtr;
    GeoUInt8PropertyMTRecPtr types = GeoUInt8Property::create();

	for (int i = 0; i < pySize(typeList); i++) {
		PyObject* pyType = PyList_GetItem(typeList, i);

		string stype = PyString_AsString(pyType);
		int type = toGLConst(stype);
		if (type == -1) {
			PyErr_SetString(err, (stype + " is not a valid type").c_str() );
			return NULL;
		}

		types->addValue(type);
	}

    geo->setTypes(types);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setPositions(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;

	GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();

    int ld = getListDepth(vec);
    string tname = vec->ob_type->tp_name;

    if (ld == 1) {
#ifndef WITHOUT_NUMPY
        if (tname == "numpy.ndarray") feed1D3np<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vec, pos);
        else
#endif
        feed1D3<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vec, pos);
    } else if (ld == 2) {
#ifndef WITHOUT_NUMPY
        if (tname == "numpy.ndarray") feed2Dnp<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vec, pos);
        else
#endif
        feed2D<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vec, pos);
    } else if (ld == 3) {
        for(Py_ssize_t i = 0; i < PyList_Size(vec); i++) {
            PyObject* vecList = PyList_GetItem(vec, i);
            string tname = vecList->ob_type->tp_name;
#ifndef WITHOUT_NUMPY
            if (tname == "numpy.ndarray") feed2Dnp<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vecList, pos);
            else
#endif
            feed2D<GeoPnt3fPropertyMTRecPtr, Pnt3d>(vecList, pos);
        }
    } else {
        string e = "VRPyGeometry::setPositions - bad argument, ld is " + toString(ld);
        PyErr_SetString(err, e.c_str());
        return NULL;
    }

    self->objPtr->setPositions(pos);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setNormals(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;

    GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
    string tname = vec->ob_type->tp_name;
    int ld = getListDepth(vec);

    if (ld == 1) {
#ifndef WITHOUT_NUMPY
        if (tname == "numpy.ndarray") feed1D3np<GeoVec3fPropertyMTRecPtr, Vec3d>( vec, norms);
        else
#endif
        feed1D3<GeoVec3fPropertyMTRecPtr, Vec3d>( vec, norms);
    } else if (ld == 2) {
#ifndef WITHOUT_NUMPY
        if (tname == "numpy.ndarray") feed2Dnp<GeoVec3fPropertyMTRecPtr, Vec3d>( vec, norms);
        else
#endif
        feed2D<GeoVec3fPropertyMTRecPtr, Vec3d>( vec, norms);
    } else {
        string e = "VRPyGeometry::setNormals - bad argument, ld is " + toString(ld);
        PyErr_SetString(err, e.c_str());
        return NULL;
    }

    self->objPtr->setNormals(norms);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setColors(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    int b = 1;
    if (! PyArg_ParseTuple(args, "O|i", &vec, &b)) return NULL;
    VRGeometryPtr geo = (VRGeometryPtr) self->objPtr;

    GeoVec4fPropertyMTRecPtr cols = GeoVec4fProperty::create();
    string tname = vec->ob_type->tp_name;
#ifndef WITHOUT_NUMPY
    if (tname == "numpy.ndarray") feed2Dnp<GeoVec4fPropertyMTRecPtr, Vec4d>( vec, cols);
    else
#endif
    feed2D<GeoVec4fPropertyMTRecPtr, Color4f>( vec, cols);

    geo->setColors(cols, b);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setLengths(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    if (!PyArg_ParseTuple(args, "O", &vec)) return NULL;
    VRGeometryPtr geo = (VRGeometryPtr) self->objPtr;

    GeoUInt32PropertyMTRecPtr lens = GeoUInt32Property::create();
    feed1D<GeoUInt32PropertyMTRecPtr>(vec, lens);
    geo->setLengths(lens);

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setIndices(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;

    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();
    string tname = vec->ob_type->tp_name;

    int ld = getListDepth(vec);
    if (ld == 1) {
#ifndef WITHOUT_NUMPY
        if (tname == "numpy.ndarray") feed1Dnp<GeoUInt32PropertyMTRecPtr>( vec, inds);
        else
#endif
        feed1D<GeoUInt32PropertyMTRecPtr>( vec, inds );
        self->objPtr->setIndices(inds, true);
    } else if (ld == 2) {
        GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
        for(Py_ssize_t i = 0; i < PyList_Size(vec); i++) {
            PyObject* vecList = PyList_GetItem(vec, i);
            feed1D<GeoUInt32PropertyMTRecPtr>( vecList, inds );
            lengths->addValue(PyList_Size(vecList));
        }
        self->objPtr->setIndices(inds);
        self->objPtr->setLengths(lengths);
    } else { PyErr_SetString(err, "VRPyGeometry::setIndices - bad argument"); return NULL; }


    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setTexCoords(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    PyObject* vec;
    int channel = 0;
    int doIndexFix = false;
    if (! PyArg_ParseTuple(args, "O|ii", &vec, &channel, &doIndexFix)) return NULL;

    if (pySize(vec) == 0) {
        GeoVec2fPropertyMTRecPtr tc = GeoVec2fProperty::create();
        self->objPtr->setTexCoords(tc, channel, doIndexFix);
        Py_RETURN_TRUE;
    }

    int vN = pySize(PyList_GetItem(vec,0));

    //cout << "VRPyGeometry::setTexCoords " << VRPyVec3f::check(PyList_GetItem(vec,0)) << endl;

    if (vN == 2) {
        GeoVec2fPropertyMTRecPtr tc = GeoVec2fProperty::create();
        feed2D<GeoVec2fPropertyMTRecPtr, Vec2d>(vec, tc);
        self->objPtr->setTexCoords(tc, channel, doIndexFix);
    }

    if (vN == 3) {
        GeoVec3fPropertyMTRecPtr tc = GeoVec3fProperty::create();
        feed2D<GeoVec3fPropertyMTRecPtr, Vec3d>(vec, tc);
        self->objPtr->setTexCoords(tc, channel, doIndexFix);
    }

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::getPositions(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getPositions - Mesh is invalid"); return NULL; }

    GeoVectorProperty* pos = self->objPtr->getMesh()->geo->getPositions();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (unsigned int i=0; i<pos->size(); i++) {
        Vec3d v;
        pos->getValue(v,i);
        PyObject* pv = toPyObject(v);
        // append to list
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getTypes(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getNormals - Mesh is invalid"); return NULL; }

    GeoIntegralProperty* types = self->objPtr->getMesh()->geo->getTypes();
    if (types == 0) return PyList_New(0);
    PyObject* res = PyList_New(types->size());

    for (unsigned int i=0; i<types->size(); i++) {
        int v;
        types->getValue(v,i);
        PyList_SetItem(res, i, PyInt_FromLong(v));
    }

    return res;
}

PyObject* VRPyGeometry::getLengths(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getNormals - Mesh is invalid"); return NULL; }

    GeoIntegralProperty* lengths = self->objPtr->getMesh()->geo->getLengths();
    if (lengths == 0) return PyList_New(0);
    PyObject* res = PyList_New(lengths->size());

    for (unsigned int i=0; i<lengths->size(); i++) {
        int v;
        lengths->getValue(v,i);
        PyList_SetItem(res, i, PyInt_FromLong(v));
    }

    return res;
}

PyObject* VRPyGeometry::getNormals(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getNormals - Mesh is invalid"); return NULL; }

    GeoVectorProperty* pos = self->objPtr->getMesh()->geo->getNormals();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (unsigned int i=0; i<pos->size(); i++) {
        Vec3d v;
        pos->getValue(v,i);
        PyObject* pv = toPyObject(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getColors(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getColors - Mesh is invalid"); return NULL; }

    GeoVectorProperty* pos = self->objPtr->getMesh()->geo->getColors();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (unsigned int i=0; i<pos->size(); i++) {
        Vec3d v;
        pos->getValue(v,i);
        PyObject* pv = toPyObject(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getIndices(VRPyGeometry* self) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getIndices - Mesh is invalid"); return NULL; }

    GeoIntegralProperty* pos = self->objPtr->getMesh()->geo->getIndices();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (unsigned int i=0; i<pos->size(); i++) {
        int v;
        pos->getValue(v,i);
        PyObject* pv = PyInt_FromLong(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getTexCoords(VRPyGeometry* self, PyObject *args) {
    if (!self->valid()) return NULL;
    if (self->objPtr->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getTexCoords - Mesh is invalid"); return NULL; }

    int channel = 0;
    if (!PyArg_ParseTuple(args, "|i", &channel)) return NULL;

    GeoVectorProperty* tc = 0;
    if (channel == 0) tc = self->objPtr->getMesh()->geo->getTexCoords();
    if (channel == 1) tc = self->objPtr->getMesh()->geo->getTexCoords1();
    if (channel == 2) tc = self->objPtr->getMesh()->geo->getTexCoords2();
    if (channel == 3) tc = self->objPtr->getMesh()->geo->getTexCoords3();
    if (channel == 4) tc = self->objPtr->getMesh()->geo->getTexCoords4();
    if (channel == 5) tc = self->objPtr->getMesh()->geo->getTexCoords5();
    if (channel == 6) tc = self->objPtr->getMesh()->geo->getTexCoords6();
    if (channel == 7) tc = self->objPtr->getMesh()->geo->getTexCoords7();

    if (tc == 0) return PyList_New(0);
    PyObject* res = PyList_New(tc->size());

    for (unsigned int i=0; i<tc->size(); i++) {
        Vec2d v;
        tc->getValue(v,i);
        PyObject* pv = toPyObject(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

