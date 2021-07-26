#ifndef VRPYSPRITE_H_INCLUDED
#define VRPYSPRITE_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/geometry/sprite/VRSprite.h"

struct VRPySprite : VRPyBaseT<OSG::VRSprite> {
    static PyMethodDef methods[];
};

#endif // VRPYSPRITE_H_INCLUDED
