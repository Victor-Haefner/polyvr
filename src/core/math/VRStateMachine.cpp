#include "VRStateMachine.h"

using namespace OSG;

VRStateMachine::VRStateMachine() {}
VRStateMachine::~VRStateMachine() {}

VRStateMachinePtr VRStateMachine::create() { return VRStateMachinePtr( new VRStateMachine() ); }

