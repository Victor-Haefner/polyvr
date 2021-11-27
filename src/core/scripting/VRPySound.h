#ifndef VRPYSOUND_H_INCLUDED
#define VRPYSOUND_H_INCLUDED

#include "VRPyObject.h"
#include "core/scene/sound/VRSoundManager.h"
#include "core/scene/sound/VRSound.h"
#include "core/scene/sound/VRMicrophone.h"

struct VRPySoundManager : VRPyBaseT<OSG::VRSoundManager> {
    static PyMethodDef methods[];
};

struct VRPySound : VRPyBaseT<OSG::VRSound> {
    static PyMethodDef methods[];
};

struct VRPyMicrophone : VRPyBaseT<OSG::VRMicrophone> {
    static PyMethodDef methods[];
};

#endif // VRPYSOCKET_H_INCLUDED
