#ifndef VRPYSOUND_H_INCLUDED
#define VRPYSOUND_H_INCLUDED

#include "VRPyObject.h"
#include "core/scene/VRSoundManager.h"

struct VRPySound : VRPyBaseT<OSG::VRSoundManager> {
    static PyMethodDef methods[];

    static PyObject* play(VRPySound* self, PyObject* args);
    static PyObject* stop(VRPySound* self, PyObject* args);
    static PyObject* stopAllSounds(VRPySound* self);
    static PyObject* setVolume(VRPySound* self, PyObject* args);
    static PyObject* playSinus(VRPySound* self, PyObject* args);
};

#endif // VRPYSOCKET_H_INCLUDED
