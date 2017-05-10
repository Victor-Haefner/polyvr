#ifndef VRPYONTOLOGY_H_INCLUDED
#define VRPYONTOLOGY_H_INCLUDED

#include "VROntology.h"
#include "VRReasoner.h"
#include "core/scripting/VRPyBase.h"

struct VRPyProperty : VRPyBaseT<OSG::VRProperty> {
    static PyMethodDef methods[];
    static PyObject* toString(VRPyProperty* self);
    static PyObject* getName(VRPyProperty* self);
    static PyObject* getType(VRPyProperty* self);
    static PyObject* getValue(VRPyProperty* self);
};

struct VRPyConcept : VRPyBaseT<OSG::VRConcept> {
    static PyMethodDef methods[];
    static PyObject* toString(VRPyConcept* self);
    static PyObject* getName(VRPyConcept* self);
    static PyObject* getProperty(VRPyConcept* self, PyObject* args);
    static PyObject* getProperties(VRPyConcept* self);
    static PyObject* addProperty(VRPyConcept* self, PyObject* args);
    static PyObject* append(VRPyConcept* self, PyObject* args);
};

struct VRPyOntologyRule : VRPyBaseT<OSG::VROntologyRule> {
    static PyMethodDef methods[];
    static PyObject* toString(VRPyOntologyRule* self);
};

struct VRPyEntity : VRPyBaseT<OSG::VREntity> {
    static PyMethodDef methods[];
    static PyObject* toString(VRPyEntity* self);
    static PyObject* getName(VRPyEntity* self);
    static PyObject* getConcept(VRPyEntity* self);
    static PyObject* getProperties(VRPyEntity* self, PyObject* args);
    static PyObject* set(VRPyEntity* self, PyObject* args);
    static PyObject* add(VRPyEntity* self, PyObject* args);
    static PyObject* clear(VRPyEntity* self, PyObject* args);
    static PyObject* setVector(VRPyEntity* self, PyObject* args);
    static PyObject* addVector(VRPyEntity* self, PyObject* args);
    static PyObject* get(VRPyEntity* self, PyObject* args);
    static PyObject* getVector(VRPyEntity* self, PyObject* args);
    static PyObject* getAll(VRPyEntity* self, PyObject* args);
    static PyObject* getAllVector(VRPyEntity* self, PyObject* args);
    static PyObject* setSGObject(VRPyEntity* self, PyObject* args);
};

struct VRPyOntology : VRPyBaseT<OSG::VROntology> {
    static PyMethodDef methods[];
    static PyObject* open(VRPyOntology* self, PyObject* args);
    static PyObject* toString(VRPyOntology* self);
    static PyObject* getConcept(VRPyOntology* self, PyObject* args);
    static PyObject* getConcepts(VRPyOntology* self);
    static PyObject* getEntities(VRPyOntology* self, PyObject* args);
    static PyObject* addConcept(VRPyOntology* self, PyObject* args);
    static PyObject* addEntity(VRPyOntology* self, PyObject* args);
    static PyObject* getEntity(VRPyOntology* self, PyObject* args);
    static PyObject* remEntity(VRPyOntology* self, PyObject* args);
    static PyObject* remEntities(VRPyOntology* self, PyObject* args);
    static PyObject* addRule(VRPyOntology* self, PyObject* args);
    static PyObject* merge(VRPyOntology* self, PyObject* args);
    static PyObject* copy(VRPyOntology* self);
    static PyObject* process(VRPyOntology* self, PyObject* args);
};

struct VRPyReasoner : VRPyBaseT<OSG::VRReasoner> {
    static PyMethodDef methods[];
    static PyObject* process(VRPyReasoner* self, PyObject* args);
};

#endif // VRPYONTOLOGY_H_INCLUDED
