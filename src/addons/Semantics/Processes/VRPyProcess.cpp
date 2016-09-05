#include "VRPyProcess.h"
#include "core/scripting/VRPyBaseT.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "core/scripting/VRPyGraph.h"

using namespace OSG;

simpleVRPyType(Process, New_named_ptr);

PyMethodDef VRPyProcess::methods[] = {
    {"open", (PyCFunction)VRPyProcess::open, METH_VARARGS, "Open file - open(path)" },
    {"setOntology", (PyCFunction)VRPyProcess::setOntology, METH_VARARGS, "Set data from ontology - open(ontology)" },
    {"getInteractionDiagram", (PyCFunction)VRPyProcess::getInteractionDiagram, METH_NOARGS, "Return subjects interaction diagram - getInteractionDiagram()" },
    {"getBehaviorDiagram", (PyCFunction)VRPyProcess::getBehaviorDiagram, METH_VARARGS, "Return subject behavior diagram - getBehaviorDiagram( str subject )" },
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
    return VRPyGraph::fromSharedPtr( self->objPtr->getBehaviorDiagram( parseString(args) ) );
}
