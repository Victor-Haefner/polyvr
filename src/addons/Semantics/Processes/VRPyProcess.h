#ifndef VRPYPROCESS_H_INCLUDED
#define VRPYPROCESS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRProcess.h"
#include "VRProcessLayout.h"
#include "VRProcessEngine.h"

struct VRPyProcess : VRPyBaseT<OSG::VRProcess> {
    static PyMethodDef methods[];
    static PyObject* open(VRPyProcess* self, PyObject* args);
    static PyObject* setOntology(VRPyProcess* self, PyObject* args);
    static PyObject* getInteractionDiagram(VRPyProcess* self);
    static PyObject* getBehaviorDiagram(VRPyProcess* self, PyObject* args);
    static PyObject* getSubjects(VRPyProcess* self);
    static PyObject* addSubject(VRPyProcess* self, PyObject* args);
    static PyObject* addMessage(VRPyProcess* self, PyObject* args);
};

struct VRPyProcessNode : VRPyBaseT<OSG::VRProcessNode> {
    static PyMethodDef methods[];
};

struct VRPyProcessLayout : VRPyBaseT<OSG::VRProcessLayout> {
    static PyMethodDef methods[];
    static PyObject* setProcess(VRPyProcessLayout* self, PyObject* args);
    static PyObject* getElement(VRPyProcessLayout* self, PyObject* args);
    static PyObject* getElementID(VRPyProcessLayout* self, PyObject* args);
    static PyObject* getProcessNode(VRPyProcessLayout* self, PyObject* args);
    static PyObject* addElement(VRPyProcessLayout* self, PyObject* args);
    static PyObject* selectElement(VRPyProcessLayout* self, PyObject* args);
    static PyObject* setElementName(VRPyProcessLayout* self, PyObject* args);
};

struct VRPyProcessEngine : VRPyBaseT<OSG::VRProcessEngine> {
    static PyMethodDef methods[];
    static PyObject* setProcess(VRPyProcessEngine* self, PyObject* args);
    static PyObject* getProcess(VRPyProcessEngine* self);
    static PyObject* run(VRPyProcessEngine* self, PyObject* args);
    static PyObject* pause(VRPyProcessEngine* self);
    static PyObject* reset(VRPyProcessEngine* self);
};

#endif // VRPYPROCESS_H_INCLUDED
