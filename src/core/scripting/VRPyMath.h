#ifndef VRPYMATH_H_INCLUDED
#define VRPYMATH_H_INCLUDED

#include "VRPyObject.h"
#include "core/math/Expression.h"
#include "core/math/Tsdf.h"
#include "core/math/Octree.h"
#ifndef WITHOUT_LAPACKE_BLAS
#include "core/math/PCA.h"
#endif
#include "core/math/patch.h"
#include "core/math/datarow.h"
#include "core/math/OSGMathFwd.h"
#include "core/utils/xml.h"
#include "core/utils/VRSpreadsheet.h"
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>

PyObject* toPyObject(const OSG::Vec2d& v);
PyObject* toPyObject(const OSG::Vec3d& v);

struct VRPyMath {
    static PyMethodDef methods[];
    static PyObject* cos(VRPyMath* self, PyObject* args);
    static PyObject* sin(VRPyMath* self, PyObject* args);
};

struct VRPyVec2f : VRPyBaseT<OSG::Vec2d> {
    OSG::Vec2d v;
    size_t itr = 0;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;
    static PySequenceMethods sMethods;

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* normalize(VRPyVec2f* self);
    static PyObject* normalized(VRPyVec2f* self);
    static PyObject* length(VRPyVec2f* self);
    static PyObject* dot(VRPyVec2f* self, PyObject* args);
    static PyObject* cross(VRPyVec2f* self, PyObject* args);
    static PyObject* asList(VRPyVec2f* self);
    static PyObject* distance(VRPyVec2f* self, PyObject* args);

    static PyObject* add(PyObject* self, PyObject* v);
    static PyObject* sub(PyObject* self, PyObject* v);
    static PyObject* mul(PyObject* self, PyObject* f);
    static PyObject* div(PyObject* self, PyObject* f);
    static PyObject* neg(PyObject* self);
    static PyObject* abs(PyObject* self);

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static int setItem(PyObject* self, Py_ssize_t i, PyObject* val);
    static PyObject* getSlice(PyObject* self, Py_ssize_t i0, Py_ssize_t i1);

    static PyObject* iter(PyObject *self) ;
    static PyObject* iternext(PyObject *self) ;
};

struct VRPyVec3f : VRPyBaseT<OSG::Vec3d> {
    OSG::Vec3d v;
    size_t itr = 0;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;
    static PySequenceMethods sMethods;

    static PyObject* fromVector(OSG::Vec3d v);

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* normalize(VRPyVec3f* self);
    static PyObject* normalized(VRPyVec3f* self);
    static PyObject* length(VRPyVec3f* self);
    static PyObject* dot(VRPyVec3f* self, PyObject* args);
    static PyObject* cross(VRPyVec3f* self, PyObject* args);
    static PyObject* asList(VRPyVec3f* self);
    static PyObject* distance(VRPyVec3f* self, PyObject* args);

    static PyObject* add(PyObject* self, PyObject* v);
    static PyObject* sub(PyObject* self, PyObject* v);
    static PyObject* mul(PyObject* self, PyObject* f);
    static PyObject* div(PyObject* self, PyObject* f);
    static PyObject* neg(PyObject* self);
    static PyObject* abs(PyObject* self);

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static int setItem(PyObject* self, Py_ssize_t i, PyObject* val);
    static PyObject* getSlice(PyObject* self, Py_ssize_t i0, Py_ssize_t i1);

    static PyObject* iter(PyObject *self) ;
    static PyObject* iternext(PyObject *self) ;
};

struct VRPyLine : VRPyBaseT<OSG::Line> {
    OSG::Line l;

    static PyMethodDef methods[];
    static PyNumberMethods nMethods;

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* Print(PyObject* self);

    static PyObject* intersect(VRPyLine* self, PyObject *args);
    static PyObject* pos(VRPyLine* self);
    static PyObject* dir(VRPyLine* self);
};

struct VRPyExpression : VRPyBaseT<OSG::Expression> {
    static PyMethodDef methods[];
};

struct VRPyMathExpression : VRPyBaseT<OSG::MathExpression> {
    static PyMethodDef methods[];
};

struct VRPyTSDF : VRPyBaseT<OSG::TSDF> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

struct VRPyOctreeNode : VRPyBaseT<OSG::OctreeNode> {
    static PyMethodDef methods[];
};

struct VRPyOctree : VRPyBaseT<OSG::Octree> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

struct VRPyPCA : VRPyBaseT<OSG::PCA> {
    static PyMethodDef methods[];
};

struct VRPyPatch : VRPyBaseT<OSG::Patch> {
    static PyMethodDef methods[];
};

struct VRPyDatarow : VRPyBaseT<OSG::Datarow> {
    size_t itr = 0;

    static PyMethodDef methods[];
    static PySequenceMethods sMethods;

    static Py_ssize_t len(PyObject* self);
    static PyObject* getItem(PyObject* self, Py_ssize_t i);
    static int setItem(PyObject* self, Py_ssize_t i, PyObject* val);
    static PyObject* getSlice(PyObject* self, Py_ssize_t i0, Py_ssize_t i1);

    static PyObject* iter(PyObject *self) ;
    static PyObject* iternext(PyObject *self) ;
};

struct VRPyXML : VRPyBaseT<OSG::XML> {
    static PyMethodDef methods[];
};

struct VRPyXMLElement : VRPyBaseT<OSG::XMLElement> {
    static PyMethodDef methods[];
};

struct VRPySpreadsheet : VRPyBaseT<OSG::VRSpreadsheet> {
    static PyMethodDef methods[];
};

#endif // VRPYMATH_H_INCLUDED
