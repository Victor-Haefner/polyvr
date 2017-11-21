#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

simplePyType( Pose, VRPyPose::New );

PyMethodDef VRPyPose::methods[] = {
    {"pos", (PyCFunction)VRPyPose::pos, METH_VARARGS, "Get the position - [x,y,z] pos()" },
    {"dir", (PyCFunction)VRPyPose::dir, METH_VARARGS, "Get the direction - [x,y,z] dir()" },
    {"up", (PyCFunction)VRPyPose::up, METH_VARARGS, "Get the up vector - [x,y,z] up()" },
    {"set", (PyCFunction)VRPyPose::set, METH_VARARGS, "Set the pose - set([pos], [dir], [up])" },
    {"mult", (PyCFunction)VRPyPose::mult, METH_VARARGS, "Transform a vector - mult([vec])" },
    {"multInv", (PyCFunction)VRPyPose::multInv, METH_VARARGS, "Transform back a vector - multInv([vec])" },
    {"invert", (PyCFunction)VRPyPose::invert, METH_NOARGS, "Invert pose - invert()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPose::invert(VRPyPose *self) {
    self->objPtr->invert();
    Py_RETURN_TRUE;
}

PyObject* VRPyPose::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject *p, *d, *u;
    if (! PyArg_ParseTuple(args, "OOO", &p, &d, &u)) return NULL;
    return allocPtr( type, OSG::Pose::create( parseVec3dList(p), parseVec3dList(d), parseVec3dList(u) ) );
}

PyObject* VRPyPose::set(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject *p, *d, *u;
    if (! PyArg_ParseTuple(args, "OOO", &p, &d, &u)) return NULL;
    self->objPtr->set( parseVec3dList(p), parseVec3dList(d), parseVec3dList(u) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPose::pos(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->pos() );
}

PyObject* VRPyPose::dir(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->dir() );
}

PyObject* VRPyPose::up(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->up() );
}

PyObject* VRPyPose::mult(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    Pnt3d v = parseVec3d(args);
    auto m = self->objPtr->asMatrix();
    m.mult(v,v);
    return toPyTuple( Vec3d(v) );
}

PyObject* VRPyPose::multInv(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    Pnt3d v = parseVec3d(args);
    auto m = self->objPtr->asMatrix();
    m.invert();
    m.mult(v,v);
    return toPyTuple( Vec3d(v) );
}


