#ifndef VRPROCESSENGINE_H_INCLUDED
#define VRPROCESSENGINE_H_INCLUDED

#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <map>
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProcessEngine {
    public:
        struct Action {
            VRProcessNodePtr node;
            float duration;
        };

        struct Actor {
            Action current;
            vector<Action> actions;
        };

    private:
        VRProcessPtr process;
        map<int, Actor> subjects;

        VRUpdateCbPtr updateCb;
        bool running = false;

        void initialize();
        void performAction(Action);
        Action nextAction(Actor);
        void update();

        float defaultDuration = 60; //= 1s if 60fps
        float speed;

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
