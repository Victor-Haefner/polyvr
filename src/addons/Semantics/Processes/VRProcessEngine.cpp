#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/math/VRStateMachine.h"
#include "core/math/VRStateMachine.cpp"

#include <boost/bind.hpp>

using namespace OSG;

template<> string typeName(const VRProcessEnginePtr& o) { return "ProcessEngine"; }

VRProcessEngine::VRProcessEngine() {
    updateCb = VRUpdateCb::create("process engine update", boost::bind(&VRProcessEngine::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::initialize(){
    cout << "VRProcessEngine::initialize()" << endl;

    auto processSubjects = process->getSubjects();
    auto initialStates = process->getInitialStates();
    //cout << "initialStates.size() " << initialStates.size() << endl;

    for (uint i=0; i<processSubjects.size(); i++) {
        int sID = processSubjects[i]->getID();
        auto states = process->getSubjectStates(sID);
        string initialState = "";

        if (initialStates.count(processSubjects[i])){
            initialState = initialStates[processSubjects[i]]->getLabel();
        }
        //cout << "initialState " << initialState << endl;

        for (uint j=0; j<states.size(); j++) {
            auto state = states[j];
            vector<Action> actions;

            auto transitions = process->getStateTransitions(sID, state->getID());
            for (auto transition : transitions) {
                auto nextState = process->getTransitionState(transition);
                Action action(nextState->getLabel(), transition);
                actions.push_back(action);
            }

            auto transitionCB = VRFunction<float, string>::create("processTransition", boost::bind(&VRProcessEngine::Actor::transitioning, &subjects[i], _1));
            subjects[i].actions[state->getLabel()] = actions;
            subjects[i].sm.addState(state->getLabel(), transitionCB);
        }
        subjects[i].sm.setCurrentState( initialState );
        //Testing
        if( processSubjects[i] && subjects[i].sm.getCurrentState()) cout << processSubjects[i]->getLabel() << " init state: " << subjects[i].sm.getCurrentState()->getName() << endl;
    }
}

void VRProcessEngine::performAction(Action action){

}

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {

    auto processSubjects = process->getSubjects();
    auto initialStates = process->getInitialStates();

    for (uint i=0; i<processSubjects.size(); i++) {
        int sID = processSubjects[i]->getID();
        auto states = process->getSubjectStates(sID);
        string initialState = "";

        if (initialStates.count(processSubjects[i])){
            initialState = initialStates[processSubjects[i]]->getLabel();
        }
        auto currentState = subjects[i].sm.getCurrentState();
        if(currentState){
            cout << "set state " << currentState->getName() << " to ";
            subjects[i].sm.setCurrentState( initialState );
            cout << currentState->getName() << endl;
        }

    }
}

void VRProcessEngine::run(float speed) {
    VRProcessEngine::speed = speed;
    running = true;
}

void VRProcessEngine::pause() {
    running = false;
}

void VRProcessEngine::update() {
    if (!running) return;

    if(tickDuration <= 0) {
        tickDuration = 60;
        for (auto& subject : subjects) {
        auto& actor = subject.second;
        actor.sm.process(0);
        }
    }
    tickDuration-=speed;
}


vector<VRProcessNodePtr> VRProcessEngine::getCurrentStates() {
    vector<VRProcessNodePtr> res;
    for (auto& subject : subjects) {
        auto action = subject.second.current;
        if (action){
            auto sID = action->node->subject;
            auto tID = action->node->getID();
            auto tStates = process->getTransitionStates(sID, tID);
            res.push_back(tStates[0]);
        }
    }
    return res;
}
