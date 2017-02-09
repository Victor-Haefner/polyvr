#ifndef VRPYMOTOR_H_INCLUDED
#define VRPYMOTOR_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "core/scene/sound/VRSound.h"
#include "contrib/rpm/rpmTool.h"

struct VRPyMotor : VRPyBaseT<rpmTool::VRMotor> {
    static PyMethodDef methods[];

    static PyObject* load(VRPyMotor* self, PyObject* args);
    static PyObject* stopAllSounds(VRPyMotor* self);
    static PyObject* setVolume(VRPyMotor* self, PyObject* args);
    static PyObject* play(VRPyMotor* self, PyObject* args);
    static PyObject* playCurrent(VRPyMotor* self, PyObject* args);
    static PyObject* setRPM(VRPyMotor* self, PyObject* args);
    static PyObject* getRPM(VRPyMotor* self);
    static PyObject* getQueuedBuffer(VRPyMotor* self);
    static PyObject* recycleBuffer(VRPyMotor* self);
};

#endif // VRPYSOCKET_H_INCLUDED
