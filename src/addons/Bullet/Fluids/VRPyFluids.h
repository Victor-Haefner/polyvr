#ifndef VRPYFLUIDS_H_INCLUDED
#define VRPYFLUIDS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRFluids.h"
#include "VRMetaBalls.h"

// TODO VRPyFluids shall inherit from VRParticles.
// NOTE that some functions may be modified here, you'll need to override them!
struct VRPyFluids : VRPyBaseT<OSG::VRFluids> {
    static PyMethodDef methods[];

    static PyObject* getGeometry(VRPyFluids* self);
    static PyObject* spawnCuboid(VRPyFluids* self, PyObject* args);
    static PyObject* spawnEmitter(VRPyFluids* self, PyObject* args);
    static PyObject* stopEmitter(VRPyFluids* self, PyObject* args);

    static PyObject* setAmount(VRPyFluids* self, PyObject* args);
    static PyObject* setRadius(VRPyFluids* self, PyObject* args);
    static PyObject* setSimType(VRPyFluids* self, PyObject* args);
    static PyObject* setSphRadius(VRPyFluids* self, PyObject* args);
    static PyObject* setMass(VRPyFluids* self, PyObject* args);
    static PyObject* setMassByRadius(VRPyFluids* self, PyObject* args);
    static PyObject* setMassForOneLiter(VRPyFluids* self, PyObject* args);
    static PyObject* setViscosity(VRPyFluids* self, PyObject* args);
    static PyObject* setRestDensity(VRPyFluids* self, PyObject* args);
};

struct VRPyMetaBalls : VRPyBaseT<OSG::VRMetaBalls> {
    static PyMethodDef methods[];

    static PyObject* getMaterial(VRPyMetaBalls* self);
    static PyObject* getDepthMaterial(VRPyMetaBalls* self);
};

#endif // VRPYFLUIDS_H_INCLUDED
