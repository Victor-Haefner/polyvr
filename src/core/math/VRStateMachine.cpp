#include "VRStateMachine.h"

using namespace OSG;


VRStateMachine::State::State(string name, VRTransitionCbPtr t) {
    setNameSpace("State");
    setName(name);
    transition = t;
}

VRStateMachine::State::~State() {}

VRStateMachine::StatePtr VRStateMachine::State::create(string name, VRTransitionCbPtr t) { return StatePtr( new State(name, t) ); }

string VRStateMachine::State::process(const map<string, string>& params) {
    return (*transition)(params);
}


VRStateMachine::VRStateMachine(string name) {
    setNameSpace("StateMachine");
    setName(name);
}

VRStateMachine::~VRStateMachine() {}

VRStateMachinePtr VRStateMachine::create(string name) { return VRStateMachinePtr( new VRStateMachine(name) ); }

VRStateMachine::StatePtr VRStateMachine::addState(string s, VRTransitionCbPtr t) {
    auto state = State::create(s,t);
    states[s] = state;
    return state;
}

VRStateMachine::StatePtr VRStateMachine::setCurrentState(string s) { currentState = states[s]; return currentState; }
VRStateMachine::StatePtr VRStateMachine::getState(string s) { return states[s]; }
VRStateMachine::StatePtr VRStateMachine::getCurrentState() { return currentState; }

VRStateMachine::StatePtr VRStateMachine::process(const map<string, string>& params) {
    string newState = currentState->process(params);
    if (states.count(newState)) setCurrentState(newState);
    return currentState;
}
