#ifndef VRPROCESSENGINE_H_INCLUDED
#define VRPROCESSENGINE_H_INCLUDED

#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/VRStateMachine.h"
#include <map>
#include <OpenSG/OSGVector.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProcessEngine {
    public:
        struct Message {
            string message;
            string sender;
            string receiver;

            Message(string m, string s, string r) : message(m), sender(s), receiver(r) {}

            bool operator==(const Message& m) { return m.message == message && m.sender == sender; }
        };

        struct Inventory {
            vector<Message> messages;

            bool hasMessage(Message m);
            void remMessage(Message m);
        };

        struct Prerequisite {
            Message message;

            Prerequisite(Message m) : message(m) {}

            bool valid(Inventory* inventory);
        };

        struct Action {
            VRUpdateCbPtr cb;

            Action(VRUpdateCbPtr cb) : cb(cb) {}
        };

        struct Transition {
            VRProcessNodePtr sourceState;
            VRProcessNodePtr nextState;
            VRProcessNodePtr node;

            string state = "unknown";
            bool overridePrerequisites = false;
            vector<Prerequisite> prerequisites;
            vector<Action> actions;

            Transition(VRProcessNodePtr s1, VRProcessNodePtr s2, VRProcessNodePtr n) : sourceState(s1), nextState(s2), node(n) {}

            bool valid(Inventory* inventory);
        };

        struct Actor {
            map<string, vector<Transition>> transitions; // maps state name (see VRStateMachine) to possible transitions
            //map<Action, Message> sendToMessage; //maps send Action to sent message
            VRStateMachine<float> sm;
            Inventory inventory;
            string initialState = "";
            string label = "";
            VRProcessNodePtr currentState;

            Actor() : sm("ProcessActor") {}

            void checkTransitions();
            string transitioning( float t ); // performs transitions to next states

            void receiveMessage(Message message);

            void tryAdvance();

            Transition& getTransition(int tID);
        };

    private:
        VRProcessPtr process;
        map<int, Actor> subjects;
        vector<Message> processMessages;

        VRUpdateCbPtr updateCb;
        bool running = false;

        void initialize();
        void performTransition(Transition t);
        void update();

        float speed;
        float tickDuration = 60; //= 1s if 60fps

    public:
        VRProcessEngine();
        ~VRProcessEngine();

        static VRProcessEnginePtr create();

        void setProcess(VRProcessPtr p);
        VRProcessPtr getProcess();

        void reset();
        void run(float speed = 1);
        void pause();

        vector<VRProcessNodePtr> getCurrentStates();
        Transition& getTransition(int sID, int tID);

        void continueWith(VRProcessNodePtr n);
        void tryAdvance(int sID);
};

OSG_END_NAMESPACE;

/**

Concept:
    - run process in simulated environment
    - realtime simulation
    - interactive simulation

*/

#endif // VRPROCESSENGINE_H_INCLUDED
