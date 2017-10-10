#pragma once

#include "core/scripting/VRPyBase.h"
#include "addons/LeapMotion/VRLeap.h"

struct VRPyLeapFrame : VRPyBaseT<OSG::VRLeapFrame> {
    static PyMethodDef methods[];
};

struct VRPyLeap : VRPyBaseT<OSG::VRLeap> {
    static PyMethodDef methods[];

    static PyObject* registerFrameCallback(VRPyLeap* self, PyObject* args);
    static PyObject* setPose(VRPyLeap* self, PyObject* args);
    static PyObject* open(VRPyLeap* self, PyObject* args);

    static PyObject* __del__(VRPyLeap* self) {
        cout << "~VRPyLeap" << endl;
    }

};
