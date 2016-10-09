#ifndef VRPYPROCESS_H_INCLUDED
#define VRPYPROCESS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRProcess.h"
#include "VRProcessLayout.h"

struct VRPyProcess : VRPyBaseT<OSG::VRProcess> {
    static PyMethodDef methods[];
    static PyObject* open(VRPyProcess* self, PyObject* args);
    static PyObject* setOntology(VRPyProcess* self, PyObject* args);
    static PyObject* getInteractionDiagram(VRPyProcess* self);
    static PyObject* getBehaviorDiagram(VRPyProcess* self, PyObject* args);
    static PyObject* getSubjects(VRPyProcess* self);
};

struct VRPyProcessNode : VRPyBaseT<OSG::VRProcessNode> {
    static PyMethodDef methods[];
    //static PyObject* getLabel(VRPyProcessNode* self);
    //static PyObject* getID(VRPyProcessNode* self);
};

struct VRPyProcessLayout : VRPyBaseT<OSG::VRProcessLayout> {
    static PyMethodDef methods[];
    static PyObject* setProcess(VRPyProcessLayout* self, PyObject* args);
    static PyObject* getElement(VRPyProcessLayout* self, PyObject* args);
};

#endif // VRPYPROCESS_H_INCLUDED
