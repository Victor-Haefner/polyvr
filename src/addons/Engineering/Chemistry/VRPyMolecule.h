#ifndef VRPYMOLECULE_H_INCLUDED
#define VRPYMOLECULE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMolecule.h"

struct VRPyMolecule : VRPyBaseT<OSG::VRMolecule> {
    static PyMethodDef methods[];
};

#endif // VRPYMOLECULE_H_INCLUDED
