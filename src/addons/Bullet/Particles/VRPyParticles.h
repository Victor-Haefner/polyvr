#ifndef VRPYPARTICLES_H_INCLUDED
#define VRPYPARTICLES_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRParticles.h"

struct VRPyParticles : VRPyBaseT<OSG::VRParticles> {
    static PyMethodDef methods[];

    static PyObject* getGeometry(VRPyParticles* self);
};

#endif // VRPYPARTICLES_H_INCLUDED
