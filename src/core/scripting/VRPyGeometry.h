#ifndef VRPYGEOMETRY_H_INCLUDED
#define VRPYGEOMETRY_H_INCLUDED

#include "VRPyTransform.h"
#include "core/objects/geometry/VRGeometry.h"

struct VRPyGeometry : VRPyBaseT<OSG::VRGeometry> {
    static PyMethodDef methods[];
    static PyObject* fromSharedPtr(OSG::VRGeometryPtr obj);

    static PyObject* setTypes(VRPyGeometry* self, PyObject *args);
    static PyObject* setPositions(VRPyGeometry* self, PyObject *args);
    static PyObject* setNormals(VRPyGeometry* self, PyObject *args);
    static PyObject* setColors(VRPyGeometry* self, PyObject *args);
    static PyObject* setLengths(VRPyGeometry* self, PyObject *args);
    static PyObject* setIndices(VRPyGeometry* self, PyObject *args);
    static PyObject* setTexCoords(VRPyGeometry* self, PyObject *args);

    static PyObject* getTypes(VRPyGeometry* self);
    static PyObject* getLengths(VRPyGeometry* self);
    static PyObject* getPositions(VRPyGeometry* self);
    static PyObject* getNormals(VRPyGeometry* self);
    static PyObject* getColors(VRPyGeometry* self);
    static PyObject* getIndices(VRPyGeometry* self);
    static PyObject* getTexCoords(VRPyGeometry* self, PyObject *args);

    static PyObject* addVertex(VRPyGeometry* self, PyObject *args);
    static PyObject* setVertex(VRPyGeometry* self, PyObject *args);
};

#endif // VRPYGEOMETRY_H_INCLUDED
