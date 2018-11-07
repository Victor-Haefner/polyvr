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
simpleVRPyType(ProcessDiagram, 0);
simpleVRPyType(ProcessLayout, New_VRObjects_ptr);
simpleVRPyType(ProcessEngine, New_ptr);

PyMethodDef VRPyProcess::methods[] = {
    {"open", PyWrap(Process, open, "Open file - open(path)", void, string ) },
    {"setOntology", PyWrap(Process, setOntology, "Set data from ontology - open(ontology)", void, VROntologyPtr ) },
    {"getOntology", PyWrap(Process, getOntology, "Get ontology", VROntologyPtr ) },
    {"getInteractionDiagram", PyWrap(Process, getInteractionDiagram, "Return subjects interaction diagram - getInteractionDiagram()", VRProcessDiagramPtr ) },
    {"getBehaviorDiagram", PyWrap(Process, getBehaviorDiagram, "Return subject behavior diagram - getBehaviorDiagram( int ID )", VRProcessDiagramPtr, int ) },
    {"getSubjects", PyWrap(Process, getSubjects, "Return subjects - [ProcessNode] getSubjects()", vector<VRProcessNodePtr> ) },
    {"getMessages", PyWrap(Process, getMessages, "Return messages - [ProcessNode] getMessages()", vector<VRProcessNodePtr> ) },
    {"getSubjectMessages", PyWrap(Process, getSubjectMessages, "Return subject messages - [ProcessNode] getSubjectMessages(subject)", vector<VRProcessNodePtr>, int ) },
    {"getMessageSubjects", PyWrap(Process, getMessageSubjects, "Return message subjects - [ProcessNode] getMessageSubjects(message)", vector<VRProcessNodePtr>, int ) },
    {"getSubjectStates", PyWrap(Process, getSubjectStates, "Return subject actions - [ProcessNode] getSubjectActions(subject)", vector<VRProcessNodePtr>, int ) },
    {"getStateTransitions", PyWrap(Process, getStateTransitions, "Return action transitions - [ProcessNode] getActionTransitions(subject, action)", vector<VRProcessNodePtr>, int, int ) },
    //{"getTransitionStates", PyWrap(Process, getTransitionStates, "Return actions connected by a given transition - [ProcessNode] getTransitionActions(subject, transition)", vector<VRProcessNodePtr>, int, int ) },
    {"getStateMessage", PyWrap(Process, getStateMessage, "Returns the Message of a send/receive state - [ProcessNode] getStateMessage(state)", VRProcessNodePtr, VRProcessNodePtr ) },
    {"addSubject", PyWrap(Process, addSubject, "Add a new subject - ProcessNode addSubject( name )", VRProcessNodePtr, string ) },
    {"addState", PyWrap(Process, addState, "Add a new state to subject, by subject ID", VRProcessNodePtr, string, int ) },
    {"setInitialState", PyWrap(Process, setInitialState, "Set a state to initial state.", void, VRProcessNodePtr ) },
    {"addMessage", PyWrapOpt(Process, addMessage, "Add a new message between subjects or actions i and j - ProcessNode addMessage( name, int i, int j )", "0", VRProcessNodePtr, string, int, int, VRProcessDiagramPtr ) },
    {"addTransition", PyWrapOpt(Process, addTransition, "Add a new transition between actions i and j - ProcessNode addTransition( name, subject, int i, int j )", "0", VRProcessNodePtr, string, int, int, int, VRProcessDiagramPtr ) },
   {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessNode::methods[] = {
    {"getLabel", PyWrap(ProcessNode, getLabel, "Get node label", string) },
    {"getID", PyWrap(ProcessNode, getID, "Get node graph ID", int) },
    {"getEntity", PyWrap(ProcessNode, getEntity, "Get entity", VREntityPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessDiagram::methods[] = {
    {"getNodes", PyWrap(ProcessDiagram, getNodes, "Get nodes", vector<VRProcessNodePtr>) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessLayout::methods[] = {
    {"getSIDPathtool", PyWrap(ProcessLayout, getSIDPathtool, "Access path tool", VRPathtoolPtr ) },
    {"getSBDPathtool", PyWrap(ProcessLayout, getSBDPathtool, "Access path tool", VRPathtoolPtr, int ) },
    {"setProcess", PyWrap(ProcessLayout, setProcess, "Set process - setProcess( process )", void, VRProcessPtr ) },
    {"getElement", PyWrap(ProcessLayout, getElement, "Return element by ID - obj getElement( int ID )", VRObjectPtr, int ) },
    {"getElementID", PyWrap(ProcessLayout, getElementID, "Return element ID - ID getElementID( VRObjectPtr geo )", int, VRObjectPtr ) },
    {"getProcessNode", PyWrap(ProcessLayout, getProcessNode, "Return process node by ID - process node getElementID( int i )", VRProcessNodePtr, int ) },
    {"addElement", PyWrap(ProcessLayout, addElement, "Add process element", VRObjectPtr, VRProcessNodePtr) },
    {"selectElement", PyWrap(ProcessLayout, selectElement, "Select a node geometry by changing its appearance", void, VRGeometryPtr ) },
    {"setElementName", PyWrap(ProcessLayout, setElementName, "Change the name of a node", void, int, string ) },
    {"remElement", PyWrap(ProcessLayout, remElement, "Remove element", void, VRObjectPtr ) },
    {"setEngine", PyWrap(ProcessLayout, setEngine, "Set process engine", void, VRProcessEnginePtr ) },
    {"storeLayout", PyWrapOpt(ProcessLayout, storeLayout, "Store layout to file", "", void, string ) },
    {"loadLayout", PyWrapOpt(ProcessLayout, loadLayout, "Load layour from file", "", void, string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProcessEngine::methods[] = {
    {"setProcess", PyWrap(ProcessEngine, setProcess, "Set process - setProcess( process )", void, VRProcessPtr ) },
    {"getProcess", PyWrap(ProcessEngine, getProcess, "Get the current process - process getProcess()", VRProcessPtr ) },
    {"run", PyWrapOpt(ProcessEngine, run, "Run the simulation with a simulation tick speed in seconds", "1", void, float ) },
    {"reset", PyWrap(ProcessEngine, reset, "Reset simulation - reset()", void ) },
    {"pause", PyWrap(ProcessEngine, pause, "Pause simulation - pause()", void ) },
    {"getCurrentStates", PyWrap(ProcessEngine, getCurrentStates, "Current states of all subjects - [ProcessNode] getCurrentStates()", vector<VRProcessNodePtr> ) },
    {"continueWith", PyWrap(ProcessEngine, continueWith, "Continue execution with this next transition", void, VRProcessNodePtr ) },
    {NULL}  /* Sentinel */
};
