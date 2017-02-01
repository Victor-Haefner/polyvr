#ifndef VRSTATEMACHINE_H_INCLUDED
#define VRSTATEMACHINE_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRStateMachine {
    private:
    public:
        VRStateMachine();
        ~VRStateMachine();

        static VRStateMachinePtr create();
};

OSG_END_NAMESPACE;

#endif // VRSTATEMACHINE_H_INCLUDED
