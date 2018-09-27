#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"

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
    for ( int i=0; i<processSubjects.size(); i++){
        auto actions = process->getSubjectActions(processSubjects[i]->getID());
        float duration = defaultDuration;
        vector<Action> actorActions = {};
        if(!actions.size() == 0){
            for (int j=0; j<actions.size(); j++){
                auto node = actions[j];
                Action actorAction{node, duration};
                actorActions.push_back(actorAction); // &actorAction;
            }
        }
        if (actorActions.size()){
            Actor actor{actorActions[0], actorActions};
            cout << processSubjects[i]->getLabel() << ": " << actorActions[0].node->getLabel() << endl;
            subjects[i] = actor; //subjects[i] = &actor;
        }
    }
}

void VRProcessEngine::performAction(Action action){

}

VRProcessEngine::Action VRProcessEngine::nextAction(Actor actor){
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
    /*
    auto transition = process->getActionTransitions(sID, currentAction->getID())[0]; //first transition

    //determine nextaction by given transition, currentAction
    auto actions = process->getTransitionActions(sID, transition->getID());
    for (auto action : actions) {
        //if (action != currentAction) currentActions[sID] = action;     //update currentActions
    }
    */
}

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {}

void VRProcessEngine::run(float speed) {
    VRProcessEngine::speed = speed;
    //TODO: adjust action durations to speed

    for (auto& subject : subjects){
        auto actor = subject.second;
        for (auto action : actor.actions){
            auto duration = action.duration;
            duration = defaultDuration/speed;
        }
    }

    running = true;
}

void VRProcessEngine::pause() {
    running = false;
}

void VRProcessEngine::update() {
    if (!running) return;

    for (auto& subject : subjects) {
        auto& actor = subject.second;
        auto& currentAction = actor.current;
        //TODO: call specific action functions for each current action
        if (subject.second.current.duration <= 0){
            subject.second.current.duration = defaultDuration; //speed;

            subject.second.current = nextAction(subject.second);
            cout << process->getSubjects()[subject.first]->getLabel() << ": " << subject.second.current.node->getLabel() << endl;
        }
        performAction(subject.second.current); //dummy action function
        subject.second.current.duration--;
    }
}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentActions(){
    vector<VRProcessNodePtr> res;

    for (auto subject : subjects) {
        res.push_back(subject.second.current.node);
    }

    return res;
}
