#include "VRPyWaypoint.h"
#include "VRPyPose.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Waypoint, New_VRObjects_ptr);

PyMethodDef VRPyWaypoint::methods[] = {
    {"get", (PyCFunction)VRPyWaypoint::get, METH_NOARGS, "Return the pose - pose get()" },
    {"set", (PyCFunction)VRPyWaypoint::set, METH_VARARGS, "Set the waypoint either from a pose or from an object - set(obj) - set(pose)" },
    {"apply", (PyCFunction)VRPyWaypoint::apply, METH_VARARGS, "Apply the waypoint to an object - apply(obj)" },
    {"setFloorPlane", (PyCFunction)VRPyWaypoint::setFloorPlane, METH_VARARGS, "Define the floor plane - setFloorPlane(pose)" },
    {"setSize", (PyCFunction)VRPyWaypoint::setSize, METH_VARARGS, "Scale the waypoint arrow - setSize( float )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyWaypoint::get(VRPyWaypoint* self) {
    if (!self->valid()) return NULL;
    return VRPyPose::fromSharedPtr( self->objPtr->get() );
}

PyObject* VRPyWaypoint::setSize(VRPyWaypoint* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float s = 1;
    if (!PyArg_ParseTuple(args, "f", &s)) return NULL;
    self->objPtr->setSize( s );
    Py_RETURN_TRUE;
}

PyObject* VRPyWaypoint::setFloorPlane(VRPyWaypoint* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->setFloorPlane( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyWaypoint::set(VRPyWaypoint* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    if (VRPyPose::check(o)) {
        auto p = (VRPyPose*)o;
        self->objPtr->set( p->objPtr );
    } else {
        auto t = (VRPyTransform*)o;
        self->objPtr->set( t->objPtr );
    }
    Py_RETURN_TRUE;
}

PyObject* VRPyWaypoint::apply(VRPyWaypoint* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* t;
    if (!PyArg_ParseTuple(args, "O", &t)) return NULL;
    self->objPtr->apply( t->objPtr );
    Py_RETURN_TRUE;
}
