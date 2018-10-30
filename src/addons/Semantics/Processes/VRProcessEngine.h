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

            bool hasMessage(Message m) {
                for (auto& m2 : messages) if (m2 == m) return true;
                return false;
            }
        };

        struct Prerequisite {
            Message message;

            Prerequisite(Message m) : message(m) {}

            bool valid(Inventory* inventory) {
                return inventory->hasMessage(message);
            }
        };

        struct Action {
            string nextState;
            VRProcessNodePtr node;
            VRProcessNodePtr sourceState;
            float duration = 0;
            vector<Prerequisite> prerequisites;

            Action(string state, VRProcessNodePtr n) : nextState(state), node(n) {}

            bool valid(Inventory* inventory) {
                for (auto& p : prerequisites) if (!p.valid(inventory)) return false;
                return true;
            }
        };

        struct Actor {
            Action* current = 0;
            map<string, vector<Action>> actions; // maps state name (see VRStateMachine) to possible Actions
            //map<Action, Message> sendToMessage; //maps send Action to sent message
            VRStateMachine<float> sm;
            Inventory inventory;
            string initialState = "";
            string label = "";

            Actor() : sm("ProcessActor") {}

            //performs transition to next state
            string transitioning( float t ) {
                auto state = sm.getCurrentState();
                string stateName = state->getName();

                // if currently in action go to next state, else check for possible actions
                if (current) {

                    string nextState = current->nextState;
                    current = 0;
                    return nextState; // state machine goes into nextState
                } else {
                    //TODO: check if this is the last state
                    if(stateName == "End"){
                        sm.setCurrentState(initialState);
                    }
                    // check if any actions are ready to start
                    for (auto& action : actions[stateName]) {
                        if (action.valid(&inventory)) {
                            current = &action;
                            return "";
                        }
                    }
                }
                return "";
            }

            void sendMessage(string message){
/*                auto message = current.transition.msgCon.message;
                auto receiver = current.transition.msgCon.receiver;
                auto sender = current.transition.msgCon.sender.label;
                receiver.inventory.messages.push_back(Message(message, sender));
*/            }
        };

    private:
        VRProcessPtr process;
        map<int, Actor> subjects;
        vector<Message> processMessages;

        VRUpdateCbPtr updateCb;
        bool running = false;

        void initialize();
        void performAction(Action);
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
        vector<VRProcessNodePtr> getCurrentNodes();
};

OSG_END_NAMESPACE;

/**

Concept:
    - run process in simulated environment
    - realtime simulation
    - interactive simulation

*/

#endif // VRPROCESSENGINE_H_INCLUDED
