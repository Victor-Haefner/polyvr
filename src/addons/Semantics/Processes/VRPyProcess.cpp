#include "VRPyProcess.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Process, New_named_ptr);
simpleVRPyType(ProcessNode, 0);
simpleVRPyType(ProcessLayout, New_VRObjects_ptr);
simpleVRPyType(ProcessEngine, New_ptr);

PyMethodDef VRPyProcess::methods[] = {
    {"open", (PyCFunction)VRPyProcess::open, METH_VARARGS, "Open file - open(path)" },
    {"setOntology", (PyCFunction)VRPyProcess::setOntology, METH_VARARGS, "Set data from ontology - open(ontology)" },
    {"getInteractionDiagram", (PyCFunction)VRPyProcess::getInteractionDiagram, METH_NOARGS, "Return subjects interaction diagram - getInteractionDiagram()" },
    {"getBehaviorDiagram", (PyCFunction)VRPyProcess::getBehaviorDiagram, METH_VARARGS, "Return subject behavior diagram - getBehaviorDiagram( int ID )" },
    {"getSubjects", (PyCFunction)VRPyProcess::getSubjects, METH_NOARGS, "Return subjects - [ProcessNode] getSubjects()" },
    {"addSubject", (PyCFunction)VRPyProcess::addSubject, METH_VARARGS, "Add a new subject - ProcessNode addSubject( name )" },
    {"addAction", PyWrap(Process, addAction, "Add a new action to subject, by ID", VRProcessNodePtr, string, int ) },
    {"addMessage", (PyCFunction)VRPyProcess::addMessage, METH_VARARGS, "Add a new message between subjects or actions i and j - ProcessNode addMessage( name, int i, int j )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProcess::open(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->open( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyProcess::addSubject(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* n = 0;
    if (!PyArg_ParseTuple(args, "s", &n)) return NULL;
    string name = n ? n : "subject";
    return VRPyProcessNode::fromSharedPtr( self->objPtr->addSubject( name ) );
}

PyObject* VRPyProcess::addMessage(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* n = 0;
    int i, j;
    if (!PyArg_ParseTuple(args, "sii", &n, &i, &j)) return NULL;
    string name = n ? n : "message";
    return VRPyProcessNode::fromSharedPtr( self->objPtr->addMessage( name, i, j ) );
}

PyObject* VRPyProcess::setOntology(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyOntology* o = 0;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->setOntology( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyProcess::getInteractionDiagram(VRPyProcess* self) {
    if (!self->valid()) return NULL;
    return VRPyGraph::fromSharedPtr( self->objPtr->getInteractionDiagram() );
}

PyObject* VRPyProcess::getBehaviorDiagram(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    return VRPyGraph::fromSharedPtr( self->objPtr->getBehaviorDiagram( parseInt(args) ) );
}

PyObject* VRPyProcess::getSubjects(VRPyProcess* self) {
    if (!self->valid()) return NULL;
    PyObject* res = PyList_New(0);
    auto subjects = self->objPtr->getSubjects();
    for (auto s : subjects) PyList_Append(res, VRPyProcessNode::fromSharedPtr(s));
    return res;
}


PyMethodDef VRPyProcessNode::methods[] = {
    {"getLabel", PyWrap(ProcessNode, getLabel, "Get node label", string) },
    {"getID", PyWrap(ProcessNode, getID, "Get node graph ID", int) },
    {NULL}  /* Sentinel */
};


PyMethodDef VRPyProcessLayout::methods[] = {
    {"setProcess", PyWrap(ProcessLayout, setProcess, "Set process - setProcess( process )", void, VRProcessPtr ) },
    {"getElement", PyWrap(ProcessLayout, getElement, "Return element by ID - obj getElement( int ID )", VRObjectPtr, int ) },
    {"getElementID", PyWrap(ProcessLayout, getElementID, "Return element ID - ID getElementID( VRObjectPtr geo )", int, VRObjectPtr ) },
    {"getProcessNode", PyWrap(ProcessLayout, getProcessNode, "Return process node by ID - process node getElementID( int i )", VRProcessNodePtr, int ) },
    {"addElement", PyWrap(ProcessLayout, addElement, "Add process element", VRObjectPtr, VRProcessNodePtr) },
    {"selectElement", PyWrap(ProcessLayout, selectElement, "Select a node geometry by changing its appearance", void, VRGeometryPtr ) },
    {"setElementName", PyWrap(ProcessLayout, setElementName, "Change the name of a node", void, int, string ) },
    {"remElement", PyWrap(ProcessLayout, remElement, "Remove element", void, VRObjectPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessEngine::methods[] = {
    {"setProcess", (PyCFunction)VRPyProcessEngine::setProcess, METH_VARARGS, "Set process - setProcess( process )" },
    {"getProcess", (PyCFunction)VRPyProcessEngine::getProcess, METH_NOARGS, "Get the current process - process getProcess()" },
    {"run", (PyCFunction)VRPyProcessEngine::run, METH_VARARGS, "Run the simulation with a simulation speed multiplier, 1 is real time - run(float s)" },
    {"reset", (PyCFunction)VRPyProcessEngine::reset, METH_NOARGS, "Reset simulation - reset()" },
    {"pause", (PyCFunction)VRPyProcessEngine::pause, METH_NOARGS, "Pause simulation - pause()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProcessEngine::pause(VRPyProcessEngine* self) {
    if (!self->valid()) return NULL;
    self->objPtr->pause();
    Py_RETURN_TRUE;
}

PyObject* VRPyProcessEngine::reset(VRPyProcessEngine* self) {
    if (!self->valid()) return NULL;
    self->objPtr->reset();
    Py_RETURN_TRUE;
}

PyObject* VRPyProcessEngine::getProcess(VRPyProcessEngine* self) {
    if (!self->valid()) return NULL;
    return VRPyProcess::fromSharedPtr( self->objPtr->getProcess() );
}

PyObject* VRPyProcessEngine::setProcess(VRPyProcessEngine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyProcess* p = 0;
    if (!PyArg_ParseTuple(args, "O", &p)) return NULL;
    self->objPtr->setProcess( p->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyProcessEngine::run(VRPyProcessEngine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->run( parseFloat(args) );
    Py_RETURN_TRUE;
}
