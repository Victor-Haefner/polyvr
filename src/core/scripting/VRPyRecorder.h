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
    static PyObject* setMaxFrames(VRPyRecorder* self, PyObject* args);
    static PyObject* frameLimitReached(VRPyRecorder* self);
    static PyObject* get(VRPyRecorder* self, PyObject* args);
    static PyObject* setTransform(VRPyRecorder* self, PyObject* args);
    static PyObject* getFrom(VRPyRecorder* self, PyObject* args);
    static PyObject* getDir(VRPyRecorder* self, PyObject* args);
    static PyObject* getAt(VRPyRecorder* self, PyObject* args);
    static PyObject* getUp(VRPyRecorder* self, PyObject* args);
};

#endif // VRPYRECORDER_H_INCLUDED
