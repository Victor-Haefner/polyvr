#ifndef VRPROCESSENGINE_H_INCLUDED
#define VRPROCESSENGINE_H_INCLUDED

#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/math/VRMathFwd.h"
#include <map>
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProcessEngine {
    private:
        VRProcessPtr process;
        //VRStateMachineMapPtr processState;
        //map<int, VRStateMachineMapPtr> subjectStates;
        map<int,VRProcessNodePtr> currentActions;
        map<int, vector<VRProcessNodePtr>> subjectActions;

        void initialize();
        void performAction(VRProcessNodePtr);
        void nextAction(int, VRProcessNodePtr);
        void update();

    public:
        VRProcessEngine();
        ~VRProcessEngine();

        static VRProcessEnginePtr create();

        void setProcess(VRProcessPtr p);
        VRProcessPtr getProcess();

        void reset();
        void run(float speed = 1);
        void pause();

        vector<VRProcessNodePtr> getCurrentActions();
};

OSG_END_NAMESPACE;

/**

Concept:
    - run process in simulated environment
    - realtime simulation
    - interactive simulation

*/

#endif // VRPROCESSENGINE_H_INCLUDED
