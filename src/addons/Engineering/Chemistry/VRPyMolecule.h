#ifndef VRPYMOLECULE_H_INCLUDED
#define VRPYMOLECULE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMolecule.h"
#include "VRCrystal.h"

struct VRPyMolecule : VRPyBaseT<OSG::VRMolecule> {
    static PyMethodDef methods[];
};

struct VRPyCrystal : VRPyBaseT<OSG::VRCrystal> {
    static PyMethodDef methods[];
};

#endif // VRPYMOLECULE_H_INCLUDED
