#ifndef VRPROCESSENGINE_H_INCLUDED
#define VRPROCESSENGINE_H_INCLUDED

#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/VRStateMachine.h"
#include <map>
#include "core/math/OSGMathFwd.h"

typedef std::map<std::string, OSG::VRProcessNodePtr> ProcessNodeData;
ptrFctFwd( VRProcess, ProcessNodeData );

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProcessEngine {
    public:
        struct Message {
            string message;
            string sender;
            string receiver;
            VRProcessNodePtr messageNode;
            VRProcessNodePtr messageSenderTransition;

            Message(string m, string s, string r, VRProcessNodePtr node) : message(m), sender(s), receiver(r), messageNode(node) {}
            Message() {}

            bool operator==(const Message& m) { return m.message == message && m.sender == sender; }
        };

        struct Inventory {
            vector<Message> messages;

            Message getMessage(Message m);
            bool hasMessage(Message m);
            void remMessage(Message m);
        };

        struct Prerequisite {
            Message message;

            Prerequisite(Message m) : message(m) {}

            bool valid(Inventory* inventory);
        };

        struct Action {
            VRProcessCbPtr cb;
            Message message; //sends an instance of this message

            Action(VRProcessCbPtr cb, Message m) : cb(cb), message(m) {}
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
            VRStateMachine<float> sm;
            Inventory inventory;
            string initialState = "";
            string label = "";
            int actionFinished = false;
            VRProcessNodePtr currentState;
            vector<VRProcessNodePtr> traversedPath; //contains transitions the engine has chosen to traverse

            Actor() : sm("ProcessActor") {}

            void checkTransitions();
            string transitioning( float t ); // performs transitions to next states

            void receiveMessage(map<string,VRProcessNodePtr> data, Message message);
            void sendMessage(Message* message);

            void tryAdvance();
            void finishAction();

            Transition& getTransition(int tID);
        };

    private:
        VRProcessPtr process;
        map<int, Actor> subjects;

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
        VRProcessNodePtr getCurrentState(int sID);
        Transition& getTransition(int sID, int tID);
        vector<VRProcessNodePtr> getTraversedPath(int sID);

        void continueWith(VRProcessNodePtr n);
        void finishAction(int sID);
        void tryAdvance(int sID);
        void sendMessage(string msg, int sID1, int sID2);
};

OSG_END_NAMESPACE;

/**

Concept:
    - run process in simulated environment
    - realtime simulation
    - interactive simulation

*/

#endif // VRPROCESSENGINE_H_INCLUDED
