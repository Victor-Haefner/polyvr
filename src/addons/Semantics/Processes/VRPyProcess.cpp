#include "VRPyProcess.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"
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
    {NULL}  /* Sentinel */
};

PyObject* VRPyProcess::open(VRPyProcess* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->open( parseString(args) );
    Py_RETURN_TRUE;
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
    for (auto s : subjects) PyList_Append(res, VRPyProcessNode::fromObject(s));
    return res;
}

PyMethodDef VRPyProcessNode::methods[] = {
    {"getLabel", PyGetter(ProcessNode, getLabel, string), "Get node label - str getLabel()" },
    {"getID", PyGetter(ProcessNode, getID, int), "Get node graph ID - int getID()" },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessLayout::methods[] = {
    {"setProcess", (PyCFunction)VRPyProcessLayout::setProcess, METH_VARARGS, "Set process - setProcess(process)" },
    {"getElement", (PyCFunction)VRPyProcessLayout::getElement, METH_VARARGS, "Return element i - obj getElement(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProcessLayout::getElement(VRPyProcessLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i=0;
    if (!PyArg_ParseTuple(args, "i", &i)) return NULL;
    return VRPyTypeCaster::cast( self->objPtr->getElement( i ) );
}

PyObject* VRPyProcessLayout::setProcess(VRPyProcessLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* p = 0;
    if (!PyArg_ParseTuple(args, "O", &p)) return NULL;
    VRProcess::DiagramPtr diag = dynamic_pointer_cast<VRProcess::Diagram>(p->objPtr);
    self->objPtr->setProcess( diag );
    Py_RETURN_TRUE;
}

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
