#ifndef VRPYTEXTUREMOSAIC_H_INCLUDED
#define VRPYTEXTUREMOSAIC_H_INCLUDED

#include "VRPyImage.h"
#include "core/objects/material/VRTextureMosaic.h"

struct VRPyTextureMosaic : VRPyBaseT<OSG::VRTextureMosaic> {
    static PyMethodDef methods[];
};

#endif // VRPYTEXTUREMOSAIC_H_INCLUDED
