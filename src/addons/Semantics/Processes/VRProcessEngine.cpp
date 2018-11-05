#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/math/VRStateMachine.h"
#include "core/math/VRStateMachine.cpp"

#include <boost/bind.hpp>

using namespace OSG;

template<> string typeName(const VRProcessEnginePtr& o) { return "ProcessEngine"; }

// ----------- process engine prerequisites --------------

bool VRProcessEngine::Inventory::hasMessage(Message m) {
    for (auto& m2 : messages) {
        cout << " VRProcessEngine::Inventory::hasMessage '" << m.message << "' and '" << m2.message << "' -> " << bool(m2 == m) << endl;
        if (m2 == m) return true;
    }
    return false;
}

// ----------- process engine prerequisites --------------

bool VRProcessEngine::Prerequisite::valid(Inventory* inventory) {
    cout << "VRProcessEngine::Prerequisite::valid check for " << message.message << endl;
    return inventory->hasMessage(message);
}

// ----------- process engine actor --------------

string VRProcessEngine::Actor::transitioning( float t ) {
    auto state = sm.getCurrentState();
    if (!state) return "";
    string stateName = state->getName();

    for (auto& transition : transitions[stateName]) { // check if any actions are ready to start
        if (transition.valid(&inventory)) {
            currentState = transition.nextState;
            for (auto& action : transition.actions) (*action.cb)();
            return transition.nextState->getLabel();
        }
    }

    return "";
}

void VRProcessEngine::Actor::receiveMessage(Message message) {
    cout << "VRProcessEngine::Actor::receiveMessage '" << message.message << "' to '" << message.receiver << "'" << endl;
    inventory.messages.push_back( message );
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
        subjects[sID].label = subject->getLabel();
    }

    for (auto& actor : subjects) {
        int sID = actor.first;

        for (auto state : process->getSubjectStates(sID)) { //for each state of this Subject create the possible Actions
            auto transitionCB = VRFunction<float, string>::create("processTransition", boost::bind(&VRProcessEngine::Actor::transitioning, &actor.second, _1));
            auto smState = actor.second.sm.addState(state->getLabel(), transitionCB);

            for (auto processTransition : process->getStateOutTransitions(sID, state->getID())) { //for each transition out of this State create Actions which lead to the next State
                //get transition requirements
                auto nextState = process->getTransitionState(processTransition);
                Transition transition(state, nextState, processTransition);

                if (processTransition->transition == RECEIVE_CONDITION) {
                    auto message = process->getTransitionMessage( processTransition );
                    if (message) {
                        auto sender = process->getMessageSender(message->getID())[0];
                        auto receiver = process->getMessageReceiver(message->getID())[0];
                        if (message && sender && receiver) {
                            Message m(message->getLabel(), sender->getLabel(), receiver->getLabel());
                            transition.prerequisites.push_back( Prerequisite(m) );
                        }
                    }
                } else if (processTransition->transition == SEND_CONDITION) {
                    auto message = process->getTransitionMessage( processTransition );
                    if (message) {
                        auto sender = process->getMessageSender(message->getID())[0];
                        auto receiver = process->getMessageReceiver(message->getID())[0];
                        if (sender && receiver) {
                            Message m(message->getLabel(), sender->getLabel(), receiver->getLabel());
                            Actor& rActor = subjects[receiver->getID()];
                            auto cb = VRUpdateCb::create("action", boost::bind(&VRProcessEngine::Actor::receiveMessage, rActor, m));
                            transition.actions.push_back( Action(cb) );
                        }
                    }
                }

                actor.second.transitions[state->getLabel()].push_back(transition);
            }
        }

        cout << "initialized, message count: " << processMessages.size() << endl;
    }
}




