#ifndef VRPYRECORDER_H_INCLUDED
#define VRPYRECORDER_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRRecorder.h"

struct VRPyRecorder : VRPyBaseT<OSG::VRRecorder> {
    static PyMethodDef methods[];

    static PyObject* capture(VRPyRecorder* self);
    static PyObject* setView(VRPyRecorder* self, PyObject* args);
    static PyObject* compile(VRPyRecorder* self, PyObject* args);
    static PyObject* clear(VRPyRecorder* self);
    static PyObject* getRecordingSize(VRPyRecorder* self);
    static PyObject* getRecordingLength(VRPyRecorder* self);
};

#endif // VRPYRECORDER_H_INCLUDED
