#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/math/VRStateMachine.h"
#include "core/math/VRStateMachine.cpp"

#include <algorithm>

using namespace OSG;

// ----------- process engine prerequisites --------------

bool VRProcessEngine::Inventory::hasMessage(Message m) {
    for (auto& m2 : messages) if (m2 == m) return true;
    return false;
}

VRProcessEngine::Message VRProcessEngine::Inventory::getMessage(Message m) {
    for (auto& m2 : messages) if (m2 == m) return m2;
    return Message();
}

void VRProcessEngine::Inventory::remMessage(Message m) {
    for (auto& m2 : messages) {
        if (m2 == m) {
            messages.erase(std::remove(messages.begin(), messages.end(), m2), messages.end());
            return;
        }
    }
    return;
}

// ----------- process engine prerequisite --------------

bool VRProcessEngine::Prerequisite::valid(Inventory* inventory) {
    bool b = inventory->hasMessage(message);
    return b;
}

// ----------- process engine transition --------------

bool VRProcessEngine::Transition::valid(Inventory* inventory) {
    if (overridePrerequisites) { overridePrerequisites = false; state = "overridden"; return true; }
    for (auto& p : prerequisites) if (!p.valid(inventory)) { state = "invalid"; return false; }
    state = "valid";
    return true;
}

// ----------- process engine actor --------------

void VRProcessEngine::Actor::checkTransitions() {
    auto state = sm.getCurrentState();
    if (state) {
        string stateName = state->getName();
        for (auto& t : transitions[stateName]) t.valid(&inventory);
    }
}

string VRProcessEngine::Actor::transitioning( float t ) {
    auto state = sm.getCurrentState();
    if (!state) return "";
    if (currentState->waitForExternalAction && !actionFinished) return "";
    string stateName = state->getName();

    for (auto& transition : transitions[stateName]) { // check if any actions are ready to start
        if (transition.valid(&inventory)) {
            actionFinished = false; // reset flag
            currentState = transition.nextState;

            vector<Message> messages;
            for ( auto p : transition.prerequisites) {
                messages.push_back( inventory.getMessage(p.message) );
            }

            for (auto& action : transition.actions) {
                map<string,VRProcessNodePtr> data;
                data["transition_receiver"] = transition.node;
                data["transition_sender"] = messages.size() > 0 ? messages[0].messageSenderTransition : 0;
                (*action.cb)(data);
                //sendMessage(&action.message); // deprecated
            }

            for ( auto p : transition.prerequisites) {
                inventory.remMessage(p.message);
            }

            traversedPath.push_back(transition.node);
            return transition.nextState->getLabel();
        }
    }
    return "";
}

//TODO: Actor should receive all messages that were sent to him without knowing the message
void VRProcessEngine::Actor::receiveMessage(map<string,VRProcessNodePtr> data, Message message) {
    //cout << "VRProcessEngine::Actor::receiveMessage '" << message.message << "' to '" << message.receiver << "' from: " << message.sender << ", sender transition: " << message.messageSenderTransition->label << endl;
    inventory.messages.push_back( message );
}

void VRProcessEngine::Actor::sendMessage(Message* m) { // deprecated
    //cout << "VRProcessEngine::Actor::sendMessage '" << m->message << "' to '" << m->sender << "' inv: " << &inventory << ", actor: " << this << endl;
    // /*Message* message =*/ new Message(m->message, m->sender, m->receiver, m->messageNode);
}

void VRProcessEngine::Actor::tryAdvance() {
    sm.process(0);
    checkTransitions();
}

void VRProcessEngine::Actor::finishAction() { actionFinished = true; }

auto nullTransition = VRProcessEngine::Transition(0,0,0);
VRProcessEngine::Transition& VRProcessEngine::Actor::getTransition(int tID) {
    for (auto& state : transitions) {
        for (auto& t : state.second) {
            if (t.node->getID() == tID) return t;
        }
    }
    return nullTransition;
}

// ----------- process engine --------------

VRProcessEngine::VRProcessEngine() {
    updateCb = VRUpdateCb::create("process engine update", bind(&VRProcessEngine::update, this));
    VRScene::getCurrent()->addTimeoutFkt(updateCb, 0, 500);
}

VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }
void VRProcessEngine::pause() { running = false; }
void VRProcessEngine::performTransition(Transition transition) {}
void VRProcessEngine::tryAdvance(int sID) { if (subjects.count(sID)) subjects[sID].tryAdvance(); }
void VRProcessEngine::finishAction(int sID) { if (subjects.count(sID)) subjects[sID].finishAction(); }
VRProcessEngine::Transition& VRProcessEngine::getTransition(int sID, int tID) { return subjects[sID].getTransition(tID); }

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

    for (auto& actor : subjects) {
        actor.second.inventory.messages.clear();
        actor.second.traversedPath.clear();
    }
}

void VRProcessEngine::update() {
    if (!running) return;
    for (auto& subject : subjects) subject.second.tryAdvance();
}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentStates() {
    vector<VRProcessNodePtr> res;
    for (auto& actor : subjects) {
        auto state = actor.second.currentState;
        if (state) res.push_back(state);
    }
    return res;
}

VRProcessNodePtr VRProcessEngine::getCurrentState(int sID) {
    return subjects[sID].currentState;
}

vector<VRProcessNodePtr> VRProcessEngine::getTraversedPath(int sID){
    return subjects[sID].traversedPath;
}

void VRProcessEngine::continueWith(VRProcessNodePtr n) {
    int sID = n->subject;
    Actor& a = subjects[sID];
    for (auto& tv : a.transitions) {
        for (auto& t : tv.second) {
            if (t.node == n) {
                t.overridePrerequisites = true;
                return;
            }
        }
    }
}

void VRProcessEngine::sendMessage(string msg, int sID1, int sID2) {
    auto mNode = process->getMessage(msg, sID1, sID2);
    Message m(msg, process->getSubject(sID1)->label, process->getSubject(sID2)->label, mNode);
    subjects[sID2].inventory.messages.push_back(m); // just push message into receiver inventory
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
            auto transitionCB = VRFunction<float, string>::create("processTransition", bind(&VRProcessEngine::Actor::transitioning, &actor.second, placeholders::_1));
            auto smState = actor.second.sm.addState(state->getLabel(), transitionCB);

            for (auto processTransition : process->getStateOutTransitions(sID, state->getID())) { // for each transition out of this State create Actions which lead to the next State
                //get transition requirements
                auto nextState = process->getTransitionState(processTransition);
                Transition transition(state, nextState, processTransition);
                if (processTransition->callback) {
                    Message m;
                    //m.messageNode = processTransition;
                    transition.actions.push_back( Action(processTransition->callback, m) );
                }

                if (processTransition->transition == RECEIVE_CONDITION) {
                    auto messages = process->getTransitionMessages( processTransition );
                    for (auto message : messages) {
                        auto sender = process->getMessageSender(message->getID())[0];
                        auto receiver = process->getMessageReceiver(message->getID())[0];
                        if (message && sender && receiver) {
                            Message m(message->getLabel(), sender->getLabel(), receiver->getLabel(), state);
                            transition.prerequisites.push_back( Prerequisite(m) );
                        }
                    }
                } else if (processTransition->transition == SEND_CONDITION) {
                    auto messages = process->getTransitionMessages( processTransition );
                    for (auto message : messages) {
                        auto sender = process->getMessageSender(message->getID())[0];
                        auto receiver = process->getMessageReceiver(message->getID())[0];
                        if (sender && receiver) {
                            Message m(message->getLabel(), sender->getLabel(), receiver->getLabel(), state); // TODO: add data artefact pointer?
                            m.messageSenderTransition = processTransition;
                            Actor& rActor = subjects[receiver->getID()];
                            auto cb = VRProcessCb::create("action", bind(&VRProcessEngine::Actor::receiveMessage, &rActor, placeholders::_1, m)); // better to trigger receiveMessage during receive transition of the receiver actor
                            transition.actions.push_back( Action(cb, m) );
                        }
                    }
                }

                actor.second.transitions[state->getLabel()].push_back(transition);
            }
        }
    }
}




