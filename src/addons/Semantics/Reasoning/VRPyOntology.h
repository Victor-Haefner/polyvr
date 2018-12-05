#ifndef VRPYONTOLOGY_H_INCLUDED
#define VRPYONTOLOGY_H_INCLUDED

#include "VROntology.h"
#include "VRReasoner.h"
#include "core/scripting/VRPyBase.h"

struct VRPyProperty : VRPyBaseT<OSG::VRProperty> {
    static PyMethodDef methods[];
};

struct VRPyConcept : VRPyBaseT<OSG::VRConcept> {
    static PyMethodDef methods[];
};

struct VRPyOntologyRule : VRPyBaseT<OSG::VROntologyRule> {
    static PyMethodDef methods[];
};

struct VRPyEntity : VRPyBaseT<OSG::VREntity> {
    static PyMethodDef methods[];
};

struct VRPyOntology : VRPyBaseT<OSG::VROntology> {
    static PyMethodDef methods[];
};

struct VRPyReasoner : VRPyBaseT<OSG::VRReasoner> {
    static PyMethodDef methods[];
};

#endif // VRPYONTOLOGY_H_INCLUDED
