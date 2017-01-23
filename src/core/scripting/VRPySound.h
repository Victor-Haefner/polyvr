#ifndef VRPYSOUND_H_INCLUDED
#define VRPYSOUND_H_INCLUDED

#include "VRPyObject.h"
#include "core/scene/sound/VRSound.h"

struct VRPySound : VRPyBaseT<OSG::VRSound> {
    static PyMethodDef methods[];

    static PyObject* play(VRPySound* self, PyObject* args);
    static PyObject* stop(VRPySound* self, PyObject* args);
    static PyObject* stopAllSounds(VRPySound* self);
    static PyObject* setVolume(VRPySound* self, PyObject* args);
    static PyObject* synthesize(VRPySound* self, PyObject* args);
    static PyObject* synthBuffer(VRPySound* self, PyObject* args);
    static PyObject* getQueuedBuffer(VRPySound* self);
    static PyObject* recycleBuffer(VRPySound* self);
};

#endif // VRPYSOCKET_H_INCLUDED
