#ifndef VRPYROBOTARM_H_INCLUDED
#define VRPYROBOTARM_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRRobotArm.h"

struct VRPyRobotArm : VRPyBaseT<OSG::VRRobotArm> {
    static PyMethodDef methods[];

    static PyObject* setParts(VRPyRobotArm* self, PyObject* args);
    static PyObject* setAngleOffsets(VRPyRobotArm* self, PyObject* args);
    static PyObject* setAngleDirections(VRPyRobotArm* self, PyObject* args);
    static PyObject* setAxis(VRPyRobotArm* self, PyObject* args);
    static PyObject* setLengths(VRPyRobotArm* self, PyObject* args);
    static PyObject* moveTo(VRPyRobotArm* self, PyObject* args);
    static PyObject* setGrab(VRPyRobotArm* self, PyObject* args);
    static PyObject* toggleGrab(VRPyRobotArm* self);
    static PyObject* setAngles(VRPyRobotArm* self, PyObject* args);
    static PyObject* getAngles(VRPyRobotArm* self);
    static PyObject* setPath(VRPyRobotArm* self, PyObject* args);
    static PyObject* getPath(VRPyRobotArm* self);
    static PyObject* moveOnPath(VRPyRobotArm* self, PyObject* args);
};

#endif // VRPYROBOTARM_H_INCLUDED
