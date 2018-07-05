#ifndef VRPYAMLLOADER_H_INCLUDED
#define VRPYAMLLOADER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRAMLLoader.h"

struct VRPyAMLLoader : VRPyBaseT<OSG::VRAMLLoader> {
    static PyMethodDef methods[];
};

#endif // VRPYAMLLOADER_H_INCLUDED
