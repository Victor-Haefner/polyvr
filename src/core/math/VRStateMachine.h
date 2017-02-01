#ifndef VRSTATEMACHINE_H_INCLUDED
#define VRSTATEMACHINE_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRName.h"
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRStateMachine : public VRName {
    public:
        typedef VRFunction< map<string, string>, string > VRTransitionCb;
        typedef shared_ptr<VRTransitionCb> VRTransitionCbPtr;

        class State : public VRName {
            private:
                VRTransitionCbPtr transition;

            public:
                State(string name, VRTransitionCbPtr t);
                ~State();

                static shared_ptr<State> create(string name, VRTransitionCbPtr t);

                string process(const map<string, string>& parameters);
        };

        typedef shared_ptr<State> StatePtr;

    private:
        StatePtr currentState;
        map<string, StatePtr> states;

    public:
        VRStateMachine( string name );
        ~VRStateMachine();

        static VRStateMachinePtr create(string name = "StateMachine");

        StatePtr addState(string s, VRTransitionCbPtr t);
        StatePtr getState(string s);
        StatePtr setCurrentState(string s);
        StatePtr getCurrentState();

        StatePtr process(const map<string, string>& parameters);
};

OSG_END_NAMESPACE;

#endif // VRSTATEMACHINE_H_INCLUDED
