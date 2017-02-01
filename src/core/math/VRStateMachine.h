#ifndef VRSTATEMACHINE_H_INCLUDED
#define VRSTATEMACHINE_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRStateMachine {
    private:
        string currentState;
        vector<string> states;

    public:
        VRStateMachine();
        ~VRStateMachine();

        static VRStateMachinePtr create();

        void addState(string s);
        void setState(string s);
        string getState();
};

OSG_END_NAMESPACE;

#endif // VRSTATEMACHINE_H_INCLUDED
