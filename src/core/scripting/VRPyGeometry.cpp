#include "VRPyGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRVideo.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "VRPyBaseT.h"
#include "VRPyMaterial.h"
#include "VRPyTypeCaster.h"

#define NO_IMPORT_ARRAY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarraytypes.h"
#include "numpy/ndarrayobject.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

template<> PyTypeObject VRPyBaseT<OSG::VRGeometry>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Geometry",             /*tp_name*/
    sizeof(VRPyGeometry),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRGeometry binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyGeometry::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMethodDef VRPyGeometry::methods[] = {
    {"setType", (PyCFunction)VRPyGeometry::setType, METH_VARARGS, "set geometry type - setType(type)" },
    {"setTypes", (PyCFunction)VRPyGeometry::setTypes, METH_VARARGS, "set geometry type - setTypes([type1, type2, ..])" },
    {"setPositions", (PyCFunction)VRPyGeometry::setPositions, METH_VARARGS, "set geometry positions - setPositions(list of (list of [x,y,z]))" },
    {"setNormals", (PyCFunction)VRPyGeometry::setNormals, METH_VARARGS, "set geometry normals - setNormals([[x,y,z], ...])" },
    {"setColors", (PyCFunction)VRPyGeometry::setColors, METH_VARARGS, "set geometry colors - setColors([[x,y,z], ...])" },
    {"setIndices", (PyCFunction)VRPyGeometry::setIndices, METH_VARARGS, "set geometry indices - setIndices(int[])" },
    {"setTexCoords", (PyCFunction)VRPyGeometry::setTexCoords, METH_VARARGS, "set geometry texture coordinates - setTexCoords(tc[])" },
    {"setTexture", (PyCFunction)VRPyGeometry::setTexture, METH_VARARGS, "set texture from file - setTexture(path)" },
    {"setMaterial", (PyCFunction)VRPyGeometry::setMaterial, METH_VARARGS, "set material" },
    {"getPositions", (PyCFunction)VRPyGeometry::getPositions, METH_NOARGS, "get geometry positions" },
    {"getNormals", (PyCFunction)VRPyGeometry::getNormals, METH_NOARGS, "get geometry normals" },
    {"getColors", (PyCFunction)VRPyGeometry::getColors, METH_NOARGS, "get geometry colors" },
    {"getIndices", (PyCFunction)VRPyGeometry::getIndices, METH_NOARGS, "get geometry indices" },
    {"getTexCoords", (PyCFunction)VRPyGeometry::getTexCoords, METH_NOARGS, "get geometry texture coordinates" },
    {"getTexture", (PyCFunction)VRPyGeometry::getTexture, METH_NOARGS, "get texture filepath" },
    {"getMaterial", (PyCFunction)VRPyGeometry::getMaterial, METH_NOARGS, "get material" },
    {"merge", (PyCFunction)VRPyGeometry::merge, METH_VARARGS, "Merge another geometry into this one" },
    {"setPrimitive", (PyCFunction)VRPyGeometry::setPrimitive, METH_VARARGS, "Set geometry to primitive" },
    {"setVideo", (PyCFunction)VRPyGeometry::setVideo, METH_VARARGS, "Set video texture - setVideo(path)" },
    {"playVideo", (PyCFunction)VRPyGeometry::playVideo, METH_VARARGS, "Play the video texture from t0 to t1 - playVideo(t0, t1, speed)" },
    {"decimate", (PyCFunction)VRPyGeometry::decimate, METH_VARARGS, "Decimate geometry by collapsing a fraction of edges - decimate(f)" },
    {"setRandomColors", (PyCFunction)VRPyGeometry::setRandomColors, METH_NOARGS, "Set a random color for each vertex" },
    {"removeDoubles", (PyCFunction)VRPyGeometry::removeDoubles, METH_VARARGS, "Remove double vertices" },
    {"makeUnique", (PyCFunction)VRPyGeometry::makeUnique, METH_NOARGS, "Make the geometry data unique" },
    {"influence", (PyCFunction)VRPyGeometry::influence, METH_VARARGS, "Pass a points and value vector to influence the geometry - influence([points,f3], [values,f3], int power)" },
    {"showGeometricData", (PyCFunction)VRPyGeometry::showGeometricData, METH_VARARGS, "Enable or disable a data layer - showGeometricData(string type, bool)\n layers are: ['Normals']" },
    {"calcSurfaceArea", (PyCFunction)VRPyGeometry::calcSurfaceArea, METH_NOARGS, "Compute and return the total surface area - flt calcSurfaceArea()" },
    {NULL}  /* Sentinel */
};

/*
Positions get/set, setTypes
*/

int getListDepth(PyObject* o) {
    string tname;
	tname = o->ob_type->tp_name;
	if (tname == "list") if (PyList_Size(o) > 0) return getListDepth(PyList_GetItem(o, 0))+1;
	if (tname == "numpy.ndarray") return 2;
	return 0;
}

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

template<class T, class t>
void feed2D(PyObject* o, T& vec) {
    PyObject *pi, *pj;
    t tmp;
    Py_ssize_t N = PyList_Size(o);

    for (Py_ssize_t i=0; i<N; i++) {
        pi = PyList_GetItem(o, i);
        for (Py_ssize_t j=0; j<PyList_Size(pi); j++) {
            pj = PyList_GetItem(pi, j);
            tmp[j] = PyFloat_AsDouble(pj);
        }
        vec->push_back(tmp);
    }
}

template<class T, class t>
void feed2D_v2(PyObject* o, T& vec) {
    PyObject *pi, *pj;
    t tmp;
    Py_ssize_t N = PyList_Size(o);

    for (Py_ssize_t i=0; i<N; i++) {
        pi = PyList_GetItem(o, i);
        for (Py_ssize_t j=0; j<PyList_Size(pi); j++) {
            pj = PyList_GetItem(pi, j);
            tmp[j] = PyFloat_AsDouble(pj);
        }
        vec.push_back(tmp);
    }
}

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

PyObject* VRPyGeometry::calcSurfaceArea(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::showGeometricData - Object is invalid"); return NULL; }
    return PyFloat_FromDouble( self->obj->calcSurfaceArea() );
}

PyObject* VRPyGeometry::showGeometricData(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::showGeometricData - Object is invalid"); return NULL; }
	PyObject* type;
	int b;
    if (!PyArg_ParseTuple(args, "Oi", &type, &b)) return NULL;
    self->obj->showGeometricData( PyString_AsString(type), b);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::influence(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::influence - Object is invalid"); return NULL; }
	PyObject *vP, *vV; int power; float color_coding; float dl_max;
    if (!PyArg_ParseTuple(args, "OOiff", &vP, &vV, &power, &color_coding, &dl_max)) return NULL;
    vector<OSG::Vec3f> pos;
    vector<OSG::Vec3f> vals;
    feed2D_v2<vector<OSG::Vec3f>, OSG::Vec3f>(vP, pos);
    feed2D_v2<vector<OSG::Vec3f>, OSG::Vec3f>(vV, vals);
    self->obj->influence(pos, vals, power, color_coding, dl_max);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::merge(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::merge - Object is invalid"); return NULL; }
	VRPyGeometry* geo;
    if (!PyArg_ParseTuple(args, "O", &geo)) return NULL;
    self->obj->merge(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setRandomColors(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::setRandomColors - Object is invalid"); return NULL; }
    self->obj->setRandomColors();
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::makeUnique(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::makeUnique -  Object is invalid"); return NULL; }
    self->obj->makeUnique();
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::removeDoubles(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::removeDoubles -  Object is invalid"); return NULL; }
    self->obj->removeDoubles( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::decimate(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::decimate -  Object is invalid"); return NULL; }
    self->obj->decimate( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setType(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::setType -  Object is invalid"); return NULL; }

    string stype = parseString(args);

    int type = toGLConst(stype);
    if (type == -1) {
        PyErr_SetString(err, (stype + " is not a valid type").c_str() );
        return NULL;
    }

    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;
    geo->setType(type);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setTypes(VRPyGeometry* self, PyObject *args) {
	PyObject* typeList;
    if (!PyArg_ParseTuple(args, "O", &typeList)) return NULL;

    if (self->obj == 0) {
		PyErr_SetString(err, "C Object is invalid");
		return NULL;
	}

    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;
    OSG::GeoUInt8PropertyRecPtr types = OSG::GeoUInt8Property::create();

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

/**
 * @brief Sets the vertex positions of the geometry.
 * @param args A list of lists of vertices: [[[x,y,z], [x,y,z], ...], [[x,y,z], ...]]
 * Each of the lists must contain a single type of primitives that corresponds to the
 * types provided through setTypes()
 */

PyObject* VRPyGeometry::setPositions(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::setPositions - C Object is invalid"); return NULL; }

	OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();

    int ld = getListDepth(vec);
    if (ld == 2) {
        string tname = vec->ob_type->tp_name;
        if (tname == "numpy.ndarray") feed2Dnp<OSG::GeoPnt3fPropertyRecPtr, OSG::Pnt3f>(vec, pos);
        else feed2D<OSG::GeoPnt3fPropertyRecPtr, OSG::Pnt3f>(vec, pos);
    } else if (ld == 3) {
        for(Py_ssize_t i = 0; i < PyList_Size(vec); i++) {
            PyObject* vecList = PyList_GetItem(vec, i);
            string tname = vecList->ob_type->tp_name;
            if (tname == "numpy.ndarray") feed2Dnp<OSG::GeoPnt3fPropertyRecPtr, OSG::Pnt3f>(vecList, pos);
            else feed2D<OSG::GeoPnt3fPropertyRecPtr, OSG::Pnt3f>(vecList, pos);
        }
    } else {
        string e = "VRPyGeometry::setPositions - bad argument, ld is " + toString(ld);
        PyErr_SetString(err, e.c_str());
        return NULL;
    }

    self->obj->setPositions(pos);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setNormals(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }

    OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
    string tname = vec->ob_type->tp_name;
    if (tname == "numpy.ndarray") feed2Dnp<OSG::GeoVec3fPropertyRecPtr, OSG::Vec3f>( vec, norms);
    else feed2D<OSG::GeoVec3fPropertyRecPtr, OSG::Vec3f>( vec, norms);

    self->obj->setNormals(norms);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setColors(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

    OSG::GeoVec4fPropertyRecPtr cols = OSG::GeoVec4fProperty::create();
    string tname = vec->ob_type->tp_name;
    if (tname == "numpy.ndarray") feed2Dnp<OSG::GeoVec4fPropertyRecPtr, OSG::Vec4f>( vec, cols);
    else feed2D<OSG::GeoVec4fPropertyRecPtr, OSG::Vec4f>( vec, cols);

    geo->setColors(cols, true);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setLengths(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (!PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

    OSG::GeoUInt32PropertyRecPtr lens = OSG::GeoUInt32Property::create();
    feed1D<OSG::GeoUInt32PropertyRecPtr>(vec, lens);
    geo->setLengths(lens);

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setIndices(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }

    OSG::GeoUInt32PropertyRecPtr inds = OSG::GeoUInt32Property::create();

    int ld = getListDepth(vec);
    if (ld == 1) {
        feed1D<OSG::GeoUInt32PropertyRecPtr>( vec, inds );
        self->obj->setIndices(inds);
    } else if (ld == 2) {
        OSG::GeoUInt32PropertyRecPtr lengths = OSG::GeoUInt32Property::create();
        for(Py_ssize_t i = 0; i < PyList_Size(vec); i++) {
            PyObject* vecList = PyList_GetItem(vec, i);
            feed1D<OSG::GeoUInt32PropertyRecPtr>( vecList, inds );
            lengths->addValue(PyList_Size(vecList));
        }
        self->obj->setIndices(inds);
        self->obj->setLengths(lengths);
    } else { PyErr_SetString(err, "VRPyGeometry::setIndices - bad argument"); return NULL; }


    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setTexCoords(VRPyGeometry* self, PyObject *args) {
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }

    if (pySize(vec) == 0) Py_RETURN_TRUE;
    int vN = pySize(PyList_GetItem(vec,0));

    if (vN == 2) {
        OSG::GeoVec2fPropertyRecPtr tc = OSG::GeoVec2fProperty::create();
        feed2D<OSG::GeoVec2fPropertyRecPtr, OSG::Vec2f>(vec, tc);
        self->obj->setTexCoords(tc);
    }

    if (vN == 3) {
        OSG::GeoVec3fPropertyRecPtr tc = OSG::GeoVec3fProperty::create();
        feed2D<OSG::GeoVec3fPropertyRecPtr, OSG::Vec3f>(vec, tc);
        self->obj->setTexCoords(tc);
    }

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setTexture(VRPyGeometry* self, PyObject *args) {
    PyObject* _path;
    if (! PyArg_ParseTuple(args, "O", &_path)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

    string path = PyString_AsString(_path);

    geo->getMaterial()->setTexture(path);

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::setMaterial(VRPyGeometry* self, PyObject *args) {
    PyObject* obj;
    if (! PyArg_ParseTuple(args, "O", &obj)) return NULL;
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::setMaterial: C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

	VRPyMaterial *pyMat = (VRPyMaterial*)obj;
    geo->setMaterial((OSG::VRMaterial*)pyMat->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::getPositions(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getPositions - Object is invalid"); return NULL; }
    if (self->obj->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getPositions - Mesh is invalid"); return NULL; }

    OSG::GeoVectorProperty* pos = self->obj->getMesh()->getPositions();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (uint i=0; i<pos->size(); i++) {
        OSG::Vec3f v;
        pos->getValue(v,i);
        PyObject* pv = toPyTuple(v);
        // append to list
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getNormals(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getNormals - Object is invalid"); return NULL; }
    if (self->obj->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getNormals - Mesh is invalid"); return NULL; }

    OSG::GeoVectorProperty* pos = self->obj->getMesh()->getNormals();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (uint i=0; i<pos->size(); i++) {
        OSG::Vec3f v;
        pos->getValue(v,i);
        PyObject* pv = toPyTuple(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getColors(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getColors - Object is invalid"); return NULL; }
    if (self->obj->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getColors - Mesh is invalid"); return NULL; }

    OSG::GeoVectorProperty* pos = self->obj->getMesh()->getColors();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (uint i=0; i<pos->size(); i++) {
        OSG::Vec3f v;
        pos->getValue(v,i);
        PyObject* pv = toPyTuple(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getIndices(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getIndices - Object is invalid"); return NULL; }
    if (self->obj->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getIndices - Mesh is invalid"); return NULL; }

    OSG::GeoIntegralProperty* pos = self->obj->getMesh()->getIndices();
    if (pos == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos->size());

    for (uint i=0; i<pos->size(); i++) {
        int v;
        pos->getValue(v,i);
        PyObject* pv = PyInt_FromLong(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getTexCoords(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getTexCoords - Object is invalid"); return NULL; }
    if (self->obj->getMesh() == 0) { PyErr_SetString(err, "VRPyGeometry::getTexCoords - Mesh is invalid"); return NULL; }

    OSG::GeoVectorProperty* tc = self->obj->getMesh()->getTexCoords();
    if (tc == 0) return PyList_New(0);
    PyObject* res = PyList_New(tc->size());

    for (unsigned int i=0; i<tc->size(); i++) {
        OSG::Vec2f v;
        tc->getValue(v,i);
        PyObject* pv = toPyTuple(v);
        PyList_SetItem(res, i, pv);
    }

    return res;
}

PyObject* VRPyGeometry::getTexture(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getTexture - Object is invalid"); return NULL; }

    // TODO

    return NULL;
}

PyObject* VRPyGeometry::setVideo(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

    string path = parseString(args);

    geo->getMaterial()->setVideo(path);

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::playVideo(VRPyGeometry* self, PyObject *args) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    OSG::VRGeometry* geo = (OSG::VRGeometry*) self->obj;

    OSG::Vec3f params = parseVec3f(args);

    geo->getMaterial()->getVideo()->play(0, params[0], params[1], params[2]);

    Py_RETURN_TRUE;
}

PyObject* VRPyGeometry::getMaterial(VRPyGeometry* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::getMaterial - Object is invalid"); return NULL; }

    return VRPyMaterial::fromPtr(self->obj->getMaterial());
}

PyObject* VRPyGeometry::setPrimitive(VRPyGeometry* self, PyObject *args) {
    string params = parseString(args);
    if (self->obj == 0) { PyErr_SetString(err, "VRPyGeometry::setPrimitive, Object is invalid"); return NULL; }
    string p1, p2;
    stringstream ss(params);
    ss >> p1; getline(ss, p2);
    self->obj->setPrimitive(p1, p2);
    Py_RETURN_TRUE;
}
