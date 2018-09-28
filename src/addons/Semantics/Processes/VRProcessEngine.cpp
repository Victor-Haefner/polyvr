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
    cout << "initialize()" << endl;

    //get subject actions
    auto processSubjects = process->getSubjects();
    for ( uint i=0; i<processSubjects.size(); i++) {
        Actor actor;
        subjects[i] = actor;
        int SID = processSubjects[i]->getID();

        auto states = process->getSubjectStates(SID);
        float duration = defaultDuration;

        string dummyStartState = "";

        for (int j=0; j<states.size(); j++) {
            auto state = states[j];
            vector<Action> actions;

            auto transitions = process->getStateTransitions(SID, state->getID());
            for (auto transition : transitions) {
                auto nextState = process->getTransitionState(transition);
                Action action(nextState->getLabel(), transition);
                actions.push_back(action);
            }
            //Action action{state, duration};
            // // &actorAction;
            auto transitionCB = VRFunction<float, string>::create("processTransition", boost::bind(&VRProcessEngine::Actor::transitioning, &subjects[i], _1));
            subjects[i].actions[state->getLabel()] = actions;
            subjects[i].sm.addState(state->getLabel(), transitionCB);

            dummyStartState = state->getLabel();
        }

        subjects[i].sm.setCurrentState( dummyStartState );

        /*if (actorActions.size()) {
            Actor actor{actorActions[0], actorActions};
            cout << processSubjects[i]->getLabel() << ": " << actorActions[0].node->getLabel() << endl;
            subjects[i] = actor; //subjects[i] = &actor;
        }*/

    }
}

void VRProcessEngine::performAction(Action action){

}

/*VRProcessEngine::Action VRProcessEngine::nextAction(Actor actor){
    auto& currentAction = actor.current;
    auto currentActionID = currentAction.node->getID();

    //TODO: actions can have several transitions, implement a function to check which transition (to the next node) to take
    for (int i=0; i<actor.actions.size(); i++) {
        auto nodeID = actor.actions[i].node->getID();

        if(nodeID == currentActionID){
            if(currentActionID == actor.actions.back().node->getID()) return actor.actions[0];
            else return actor.actions[i+1];

        }
    }

    //check which transition to take
    auto transition = process->getActionTransitions(sID, currentAction->getID())[0]; //first transition

    //determine nextaction by given transition, currentAction
    auto actions = process->getTransitionActions(sID, transition->getID());
    for (auto action : actions) {
        //if (action != currentAction) currentActions[sID] = action;     //update currentActions
    }
}*/

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {}

void VRProcessEngine::run(float speed) {
    VRProcessEngine::speed = speed;
    //adjust action durations to speed
    /*for (auto& subject : subjects){
        auto& actor = subject.second;
        for (auto& action : actor.actions){
            auto& duration = action.duration;
            duration = defaultDuration/speed;
        }
    }*/
    running = true;
}

void VRProcessEngine::pause() {
    running = false;
}

void VRProcessEngine::update() {
    if (!running) return;

    for (auto& subject : subjects) {
        auto& actor = subject.second;
        /*auto& currentAction = actor.current;
        //TODO: call specific action functions for each current action
        if (currentAction.duration <= 0){
            currentAction.duration = defaultDuration/speed;

            currentAction = nextAction(actor);
            cout << process->getSubjects()[subject.first]->getLabel() << ": " << currentAction.node->getLabel() << endl;
        }
        performAction(currentAction); //dummy action function
        currentAction.duration--;*/

        actor.sm.process(0);
    }
}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentActions() {
    vector<VRProcessNodePtr> res;

    for (auto& subject : subjects) {
        auto action = subject.second.current;
        if (action) res.push_back(action->node);
    }

    return res;
}
