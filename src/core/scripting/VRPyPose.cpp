#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const Pose& p) { return VRPyPose::fromObject(p); }

simplePyType( Pose, VRPyPose::New );

PyMethodDef VRPyPose::methods[] = {
    {"pos", PyCastWrap2(Pose, pos, "Get the position", Vec3d ) },
    {"dir", PyCastWrap2(Pose, dir, "Get the direction", Vec3d ) },
    {"up", PyCastWrap2(Pose, up, "Get the up vector", Vec3d ) },
    {"x", PyCastWrap2(Pose, x, "Get the x vector", Vec3d ) },
    {"scale", PyCastWrap2(Pose, scale, "Get the scale vector", Vec3d ) },
    {"setPos", PyWrap2(Pose, setPos, "Set the position", void, Vec3d ) },
    {"setDir", PyWrap2(Pose, setDir, "Set the direction", void, Vec3d ) },
    {"setUp", PyWrap2(Pose, setUp, "Set the up vector", void, Vec3d ) },
    {"setScale", PyWrap2(Pose, setScale, "Set the scale vector", void, Vec3d ) },
    {"set", PyWrap2(Pose, set, "Set the pose", void, Vec3d, Vec3d, Vec3d ) },
    {"mult", PyWrap2(Pose, transform, "Transform a vector", Vec3d, Vec3d ) },
    {"multInv", PyWrap2(Pose, transformInv, "Transform back a vector", Vec3d, Vec3d ) },
    {"multLeft", PyWrap2(Pose, multLeft, "Transform back a vector", PosePtr, PosePtr ) },
    {"multRight", PyWrap2(Pose, multRight, "Transform back a vector", PosePtr, PosePtr ) },
    {"invert", PyWrap2(Pose, invert, "Invert pose", void ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPose::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject *p, *d, *u;
    if (! PyArg_ParseTuple(args, "OOO", &p, &d, &u)) return NULL;
    return allocPtr( type, OSG::Pose::create( parseVec3dList(p), parseVec3dList(d), parseVec3dList(u) ) );
}

PyObject* VRPyPose::fromMatrix(OSG::Matrix4d m) {
    return fromObject(Pose(m));
}
