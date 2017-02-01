#include "VRStateMachine.h"

using namespace OSG;

VRStateMachine::VRStateMachine() {}
VRStateMachine::~VRStateMachine() {}

VRStateMachinePtr VRStateMachine::create() { return VRStateMachinePtr( new VRStateMachine() ); }

void VRStateMachine::addState(string s) { states.push_back(s); }
void VRStateMachine::setState(string s) { currentState = s; }
string VRStateMachine::getState() { return currentState; }
