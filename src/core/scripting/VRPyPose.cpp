#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const Pose& p) { return VRPyPose::fromObject(p); }

simplePyType( Pose, VRPyPose::New );

/*PyMethodDef VRPyPose::methods[] = {
    {"pos", PyWrap2(Pose, pos, "Get the position", Vec3d ) },
    {"dir", PyWrap2(Pose, dir, "Get the direction", Vec3d ) },
    {"up", PyWrap2(Pose, up, "Get the up vector", Vec3d ) },
    {"x", PyWrap2(Pose, x, "Get the x vector", Vec3d ) },
    {"scale", PyWrap2(Pose, scale, "Get the scale vector", Vec3d ) },
    {"setPos", PyWrap2(Pose, setPos, "Set the position", void, Vec3d ) },
    {"setDir", PyWrap2(Pose, setDir, "Set the direction", void, Vec3d ) },
    {"setUp", PyWrap2(Pose, setUp, "Set the up vector", void, Vec3d ) },
    {"setScale", PyWrap2(Pose, setScale, "Set the scale vector", void, Vec3d ) },
    {"set", PyWrapOpt2(Pose, set, "Set the pose: pos, dir, up, scale", "1 1 1", void, Vec3d, Vec3d, Vec3d, Vec3d ) },
    {"mult", PyWrapOpt2(Pose, transform, "Transform a vector", "1", Vec3d, Vec3d, bool ) },
    {"multInv", PyWrap2(Pose, transformInv, "Transform back a vector", Vec3d, Vec3d ) },
    {"multLeft", PyWrap2(Pose, multLeft, "Transform back a vector", PosePtr, PosePtr ) },
    {"multRight", PyWrap2(Pose, multRight, "Transform back a vector", PosePtr, PosePtr ) },
    {"invert", PyWrap2(Pose, invert, "Invert pose", void ) },
    {NULL}
};*/

PyMethodDef VRPyPose::methods[] = {
    {"pos", PyWrap2(Pose, pos, "Get the position", Vec3d ) },
    {"dir", PyWrap2(Pose, dir, "Get the direction", Vec3d ) },
    {"up", PyWrap2(Pose, up, "Get the up vector", Vec3d ) },
    {"x", PyWrap2(Pose, x, "Get the x vector", Vec3d ) },
    {"scale", PyWrap2(Pose, scale, "Get the scale vector", Vec3d ) },
    {"setPos", PyWrap2(Pose, setPos, "Set the position", void, const Vec3d& ) },
    {"setDir", PyWrap2(Pose, setDir, "Set the direction", void, const Vec3d& ) },
    {"setUp", PyWrap2(Pose, setUp, "Set the up vector", void, const Vec3d& ) },
    {"setScale", PyWrap2(Pose, setScale, "Set the scale vector", void, const Vec3d& ) },
    {"set", PyWrapOpt2(Pose, set, "Set the pose: pos, dir, up, scale", "1 1 1", void, const Vec3d&, const Vec3d&, const Vec3d&, const Vec3d& ) },
    {"rotate", PyWrap2(Pose, rotate, "Rotate the pose (angle, axis)", void, double, const Vec3d&) },
    {"translate", PyWrap2(Pose, translate, "Translate the pose", void, const Vec3d&) },
    {"move", PyWrap2(Pose, move, "Move the pose along dir", void, double) },
    {"mult", PyWrapOpt2(Pose, transform, "Transform a vector", "1", Vec3d, const Vec3d&, bool ) },
    {"multInv", PyWrap2(Pose, transformInv, "Transform back a vector", Vec3d, const Vec3d& ) },
    {"multLeft", PyWrap2(Pose, multLeft, "Transform back a vector", PosePtr, PosePtr ) },
    {"multRight", PyWrap2(Pose, multRight, "Transform back a vector", PosePtr, PosePtr ) },
    {"invert", PyWrap2(Pose, invert, "Invert pose", void ) },
    {"toString", PyWrap2(Pose, toString, "As string", string ) },
    {NULL}
};

PyObject* VRPyPose::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject *p, *d, *u, *s;
    p = d = u = s = 0;
    if (! PyArg_ParseTuple(args, "|OOOO", &p, &d, &u, &s)) return NULL;
    Vec3d pos = Vec3d(0,0,0);
    Vec3d dir = Vec3d(0,0,-1);
    Vec3d up  = Vec3d(0,1,0);
    Vec3d scale  = Vec3d(1,1,1);
    if (p) pos = parseVec3dList(p);
    if (d) dir = parseVec3dList(d);
    if (u) up  = parseVec3dList(u);
    if (s) scale  = parseVec3dList(s);
    return allocPtr( type, OSG::Pose::create( pos, dir, up, scale ) );
}

PyObject* VRPyPose::fromMatrix(OSG::Matrix4d m) {
    return fromObject(Pose(m));
}
