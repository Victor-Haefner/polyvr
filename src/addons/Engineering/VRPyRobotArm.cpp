#include "VRPyRobotArm.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyPath.h"

using namespace OSG;

simpleVRPyType(RobotArm, New_ptr);

PyMethodDef VRPyRobotArm::methods[] = {
    {"setParts", (PyCFunction)VRPyRobotArm::setParts, METH_VARARGS, "Set robot parts - setParts([base, upper_arm, forearm, wrist, grab, jaw1, jaw2])" },
    {"setAngleOffsets", (PyCFunction)VRPyRobotArm::setAngleOffsets, METH_VARARGS, "Set angle offset for each part - setAngleOffsets([float rad])" },
    {"setAngleDirections", (PyCFunction)VRPyRobotArm::setAngleDirections, METH_VARARGS, "Set angles rotation direction - setAngleDirections([1/-1])" },
    {"setAxis", (PyCFunction)VRPyRobotArm::setAxis, METH_VARARGS, "Set rotation axis for each part - setAxis([int a])\n a: 0 = 'x', 1 = 'y', 2 = 'z'" },
    {"setLengths", (PyCFunction)VRPyRobotArm::setLengths, METH_VARARGS, "Set kinematic lengths between joints - setLengths([base_height, upper_arm length, forearm length, grab position])" },
    {"moveTo", (PyCFunction)VRPyRobotArm::moveTo, METH_VARARGS, "Move the end effector to a certain position - moveTo([x,y,z])" },
    {"setGrab", (PyCFunction)VRPyRobotArm::setGrab, METH_VARARGS, "Set grab state - setGrab(float d)\n d: 0 is closed, 1 is open" },
    {"toggleGrab", (PyCFunction)VRPyRobotArm::toggleGrab, METH_NOARGS, "Toggle the grab - toggleGrab()" },
    {"setAngles", (PyCFunction)VRPyRobotArm::setAngles, METH_VARARGS, "Set joint angles - setAngles( angles )" },
    {"getAngles", (PyCFunction)VRPyRobotArm::getAngles, METH_NOARGS, "Get joint angles - getAngles()" },
    {"getForwardKinematics", (PyCFunction)VRPyRobotArm::getForwardKinematics, METH_VARARGS, "Get end effector pose from angles - p,d,u getForwardKinematics( angles )" },
    {"getBackwardKinematics", (PyCFunction)VRPyRobotArm::getBackwardKinematics, METH_VARARGS, "Get angles from end effector pose - angles getBackwardKinematics( p,d,u )" },
    {"setPath", (PyCFunction)VRPyRobotArm::setPath, METH_VARARGS, "Set robot path - setPath()" },
    {"getPath", (PyCFunction)VRPyRobotArm::getPath, METH_NOARGS, "Get robot path - getPath()" },
    {"moveOnPath", (PyCFunction)VRPyRobotArm::moveOnPath, METH_VARARGS, "Move robot on internal path - moveOnPath(float t0, float t1, bool loop)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRobotArm::moveOnPath(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    float t0, t1; int l;
    if (! PyArg_ParseTuple(args, "ffi", &t0, &t1, &l)) return NULL;
    self->objPtr->moveOnPath(t0,t1,l);
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::getPath(VRPyRobotArm* self) {
	if (!self->valid()) return NULL;
    return VRPyPath::fromSharedPtr( self->objPtr->getPath() );
}

PyObject* VRPyRobotArm::setPath(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    VRPyPath* path = 0;
    parseObject(args, path);
    self->objPtr->setPath( path->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::getForwardKinematics(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<float> res;
    for (auto p : prts) res.push_back( PyFloat_AsDouble(p) );
    //auto pose = self->objPtr->getForwardKinematics( res );
    //return VRPyPose::fromObj(pose);
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::getBackwardKinematics(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject* pose;
    if (! PyArg_ParseTuple(args, "O", &pose)) return NULL;
    //auto pose = self->objPtr->getForwardKinematics( res );
    //return VRPyPose::fromObj(pose);
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setAngles(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<float> res;
    for (auto p : prts) res.push_back( PyFloat_AsDouble(p) );
    self->objPtr->setAngles( res );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::getAngles(VRPyRobotArm* self) {
	if (!self->valid()) return NULL;
    auto v = self->objPtr->getAngles();
    PyObject* res = PyList_New(v.size());
    for (uint i=0; i<v.size(); i++) PyList_SetItem(res, i, PyFloat_FromDouble(v[i]));
    return res;
}

PyObject* VRPyRobotArm::toggleGrab(VRPyRobotArm* self) {
	if (!self->valid()) return NULL;
    self->objPtr->toggleGrab();
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::moveTo(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject *v1, *v2, *v3;
    if (! PyArg_ParseTuple(args, "OOO", &v1, &v2, &v3)) return NULL;
    self->objPtr->moveTo( parseVec3fList(v1), parseVec3fList(v2), parseVec3fList(v3) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setGrab(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setGrab( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setParts(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<OSG::VRTransformPtr> res;
    for (auto p : prts) res.push_back( ((VRPyTransform*)p)->objPtr );
    self->objPtr->setParts( res );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setAngleDirections(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<int> res;
    for (auto p : prts) res.push_back( PyInt_AsLong(p) );
    self->objPtr->setAngleDirections( res );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setAngleOffsets(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<float> res;
    for (auto p : prts) res.push_back( PyFloat_AsDouble(p) );
    self->objPtr->setAngleOffsets( res );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setLengths(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<float> res;
    for (auto p : prts) res.push_back( PyFloat_AsDouble(p) );
    self->objPtr->setLengths( res );
    Py_RETURN_TRUE;
}

PyObject* VRPyRobotArm::setAxis(VRPyRobotArm* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto prts = parseList(args);
    vector<int> res;
    for (auto p : prts) res.push_back( PyInt_AsLong(p) );
    self->objPtr->setAxis( res );
    Py_RETURN_TRUE;
}
