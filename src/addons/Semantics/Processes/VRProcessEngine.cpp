#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/math/VRStateMachine.h"
#include "core/math/VRStateMachine.cpp"

#include <boost/bind.hpp>

using namespace OSG;

template<> string typeName(const VRProcessEnginePtr& o) { return "ProcessEngine"; }

// ----------- process engine actor --------------

string VRProcessEngine::Actor::transitioning( float t ) {
    auto state = sm.getCurrentState();
    if (!state) return "";
    string stateName = state->getName();

    for (auto& transition : transitions[stateName]) { // check if any actions are ready to start
        if (transition.valid(&inventory)) {
            currentState = transition.nextState;
            return transition.nextState->getLabel();
        }
    }

    return "";
}

void VRProcessEngine::Actor::sendMessage(string message) {
    /*auto message = current.transition.msgCon.message;
    auto receiver = current.transition.msgCon.receiver;
    auto sender = current.transition.msgCon.sender.label;
    receiver.inventory.messages.push_back(Message(message, sender));*/
}

// ----------- process engine --------------

VRProcessEngine::VRProcessEngine() {
    updateCb = VRUpdateCb::create("process engine update", boost::bind(&VRProcessEngine::update, this));
    VRScene::getCurrent()->addTimeoutFkt(updateCb, 0, 500);
}

VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }
void VRProcessEngine::pause() { running = false; }

void VRProcessEngine::performTransition(Transition transition) {}

void VRProcessEngine::run(float s) {
    speed = s; running = true;
    VRScene::getCurrent()->dropTimeoutFkt(updateCb);
    VRScene::getCurrent()->addTimeoutFkt(updateCb, 0, speed*1000);
}

void VRProcessEngine::reset() {
    auto initialStates = process->getInitialStates();

    for (auto state : process->getInitialStates()) {
        int sID = state->subject;
        subjects[sID].currentState = state;
        subjects[sID].sm.setCurrentState( state->getLabel() );
        cout << "VRProcessEngine::reset, set initial state '" << state->getLabel() << "'" << endl;
    }

    /*for (auto subject : process->getSubjects()) {
        int sID = subject->getID();

        string initialState = "";
        if (initialStates.count(subject)) {
            initialState = initialStates[subject]->getLabel();
        }
        subjects[sID].sm.setCurrentState( initialState );
        cout << "VRProcessEngine::reset, set initial state '" << initialState << "'" << endl;
    }*/
}

void VRProcessEngine::update() {
    if (!running) return;

    for (auto& subject : subjects) {
        auto& actor = subject.second;
        actor.sm.process(0);
    }
}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentStates() {
    vector<VRProcessNodePtr> res;
    for (auto& actor : subjects) {
        auto state = actor.second.currentState;
        if (state) res.push_back(state);
    }
    return res;
}

void VRProcessEngine::initialize() {
    cout << "VRProcessEngine::initialize()" << endl;

    for (auto subject : process->getSubjects()) {
        int sID = subject->getID();
        subjects[sID] = Actor();
        Actor& actor = subjects[sID];
        actor.label = subject->getLabel();

        for (auto state : process->getSubjectStates(sID)) { //for each state of this Subject create the possible Actions
            auto transitionCB = VRFunction<float, string>::create("processTransition", boost::bind(&VRProcessEngine::Actor::transitioning, &actor, _1));
            auto smState = actor.sm.addState(state->getLabel(), transitionCB);

            for (auto processTransition : process->getStateOutTransitions(sID, state->getID())) { //for each transition out of this State create Actions which lead to the next State
                //get transition requirements
                auto nextState = process->getTransitionState(processTransition);
                Transition transition(state, nextState, processTransition);

                if (processTransition->transition == RECEIVE_CONDITION) { // if state == receive state add the receive message to transition prerequisites
                    cout << "receive state found" << endl;
                    /*bool messageExist = false;
                    auto messageNode =  process->getStateMessage(state);
                    auto receiver = subject->getLabel();

                    for (auto m : processMessages) {
                        if (m.receiver == receiver) {
                            if (m.message == messageNode->getLabel()){ //if the message already exists, add it to transition prerequisites
                                Prerequisite p(m);
                                transition.prerequisites.push_back(p);
                                messageExist = true;
                            }
                        }
                    }

                    if (!messageExist){ //create a new message instance if it doesnt exist and add it to transition prerequisites
                        auto sender = process->getMessageSender(messageNode->getID());
                        for (auto s : sender) {
                            Message m(messageNode->getLabel(), s->getLabel(), receiver);
                            Prerequisite p(m);
                            transition.prerequisites.push_back(p);
                        }
                    }*/
                } else if (processTransition->transition == SEND_CONDITION) { // add a sendMessage callback
                    cout << "send state found" << endl;
                    /*VRStateMachine<float>::VRStateEnterCbPtr sendMessageCB = VRStateMachine<float>::VRStateEnterCb::create("sendMessage", boost::bind(&VRProcessEngine::Actor::sendMessage, &subjects[sID], _1));
                    smState->setStateLeaveCB(sendMessageCB);

                    auto messageNode = process->getStateMessage(state);
                    auto receiverNodes = process->getMessageReceiver(messageNode->getID());
                    auto sender = subject->getLabel();

                    bool messageExist = false;
                    for (auto m : processMessages) { //check if the message already exists
                        if (m.sender == sender) {
                            if (m.message == messageNode->getLabel()){
                                messageExist = true;
                            }
                        }
                    }

                    if (!messageExist){ //if message doesnt exist, create one for each receiver and add to processMessages
                        auto receiver = process->getMessageReceiver(messageNode->getID());
                        for (auto r : receiver) {
                            Message m(messageNode->getLabel(), sender, r->getLabel());
                            processMessages.push_back(m);
                        }
                    }*/
                }


                actor.transitions[state->getLabel()].push_back(transition);
            }
        }

        cout << "initialized, message count: " << processMessages.size() << endl;
    }
}




