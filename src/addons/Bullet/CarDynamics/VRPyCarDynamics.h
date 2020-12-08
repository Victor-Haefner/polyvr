#ifndef VRPYCARDYNAMICS_H_INCLUDED
#define VRPYCARDYNAMICS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/objects/object/VRObject.h"
#include "CarDynamics.h"
#ifndef WITHOUT_AV
#include "CarSound/CarSound.h"
#endif
#include "VRDriver.h"

struct VRPyCarDynamics : VRPyBaseT<OSG::VRCarDynamics> {
    static PyMethodDef methods[];

    static PyObject* update(VRPyCarDynamics* self, PyObject* args);
    static PyObject* setChassis(VRPyCarDynamics* self, PyObject* args);
    static PyObject* setupSimpleWheels(VRPyCarDynamics* self, PyObject* args);
    static PyObject* setParameter(VRPyCarDynamics* self, PyObject* args);
    static PyObject* reset(VRPyCarDynamics* self, PyObject* args);
    static PyObject* getSpeed(VRPyCarDynamics* self);
    static PyObject* getAcceleration(VRPyCarDynamics* self);
    static PyObject* getRoot(VRPyCarDynamics* self);
    static PyObject* getChassis(VRPyCarDynamics* self);
    static PyObject* getWheels(VRPyCarDynamics* self);
    static PyObject* getSteering(VRPyCarDynamics* self);
    static PyObject* getThrottle(VRPyCarDynamics* self);
    static PyObject* getBreaking(VRPyCarDynamics* self);
    static PyObject* getClutch(VRPyCarDynamics* self);
    static PyObject* getRPM(VRPyCarDynamics* self);
    static PyObject* geteForce(VRPyCarDynamics* self);
    static PyObject* geteBreak(VRPyCarDynamics* self);
    static PyObject* getGear(VRPyCarDynamics* self);
    static PyObject* isRunning(VRPyCarDynamics* self);
    static PyObject* setIgnition(VRPyCarDynamics* self, PyObject* args);

    static PyObject* loadCarSound(VRPyCarDynamics* self, PyObject* args);
    static PyObject* toggleCarSound(VRPyCarDynamics* self, PyObject* args);
    static PyObject* getCarSound(VRPyCarDynamics* self);
    static PyObject* carSoundIsLoaded(VRPyCarDynamics* self);

    static PyObject* setFade(VRPyCarDynamics* self, PyObject* args);
    static PyObject* setType(VRPyCarDynamics* self, PyObject* args);
};

#ifndef WITHOUT_AV
struct VRPyCarSound : VRPyBaseT<OSG::VRCarSound> {
    static PyMethodDef methods[];
};
#endif

struct VRPyDriver : VRPyBaseT<OSG::VRDriver> {
    static PyMethodDef methods[];

    static PyObject* setCar(VRPyDriver* self, PyObject* args);
    static PyObject* followPath(VRPyDriver* self, PyObject* args);
    static PyObject* isDriving(VRPyDriver* self);
};

#endif // VRPYCARDYNAMICS_H_INCLUDED
