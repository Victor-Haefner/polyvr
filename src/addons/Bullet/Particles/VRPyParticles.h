#ifndef VRPYPARTICLES_H_INCLUDED
#define VRPYPARTICLES_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRParticles.h"

struct VRPyParticles : VRPyBaseT<OSG::VRParticles> {
    static PyMethodDef methods[];

    static PyObject* getGeometry(VRPyParticles* self);
    static PyObject* spawnCuboid(VRPyParticles* self, PyObject* args);
    static PyObject* spawnEmitter(VRPyParticles* self, PyObject* args);
    static PyObject* stopEmitter(VRPyParticles* self, PyObject* args);

    static PyObject* setAmount(VRPyParticles* self, PyObject* args);
    static PyObject* setRadius(VRPyParticles* self, PyObject* args);
    static PyObject* setMass(VRPyParticles* self, PyObject* args);
    static PyObject* setMassByRadius(VRPyParticles* self, PyObject* args);
    static PyObject* setMassForOneLiter(VRPyParticles* self, PyObject* args);
};

#endif // VRPYPARTICLES_H_INCLUDED
