#include "VRStateMachine.h"

using namespace OSG;

template<class P>
VRStateMachine<P>::State::State(string name, VRTransitionCbPtr t) {
    setNameSpace("State");
    setName(name);
    transition = t;
}

template<class P>
VRStateMachine<P>::State::~State() {}

template<class P>
typename VRStateMachine<P>::StatePtr VRStateMachine<P>::State::create(string name, VRTransitionCbPtr t) { return StatePtr( new State(name, t) ); }

template<class P>
string VRStateMachine<P>::State::process(const P& params) {
    return (*transition)(params);
}


template<class P>
VRStateMachine<P>::VRStateMachine(string name) {
    setNameSpace("StateMachine");
    setName(name);
}

template<class P>
VRStateMachine<P>::~VRStateMachine() {}

template<class P>
shared_ptr<VRStateMachine<P>> VRStateMachine<P>::create(string name) { return shared_ptr<VRStateMachine<P>>( new VRStateMachine<P>(name) ); }

template<class P>
typename VRStateMachine<P>::StatePtr VRStateMachine<P>::addState(string s, VRTransitionCbPtr t) {
    auto state = State::create(s,t);
    states[s] = state;
    return state;
}

template<class P> typename VRStateMachine<P>::StatePtr VRStateMachine<P>::setCurrentState(string s) { currentState = states[s]; return currentState; }
template<class P> typename VRStateMachine<P>::StatePtr VRStateMachine<P>::getState(string s) { return states[s]; }
template<class P> typename VRStateMachine<P>::StatePtr VRStateMachine<P>::getCurrentState() { return currentState; }

template<class P>
typename VRStateMachine<P>::StatePtr VRStateMachine<P>::process(const P& params) {
    string newState = currentState->process(params);
    if (states.count(newState)) setCurrentState(newState);
    return currentState;
}
