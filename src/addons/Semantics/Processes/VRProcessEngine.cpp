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

    /* print all actions
    for (auto subject : subjects){
        auto actor = subject.second;
        cout << "Actions: " << endl;
        for (auto action : actor.actions) cout << action.node->getLabel() << endl;
    }
    */
}

void VRProcessEngine::performAction(Action action){

}

void VRProcessEngine::nextAction(Actor actor, Action currentAction){
    auto actionID = currentAction.node->getID();
    vector<Action>& actions = actor.actions;

    for (int i=0; i<actions.size(); i++) {
        auto nodeID = actions[i].node->getID();
        if(nodeID == actionID){
            int nextID = (i+1) % (actions.size()-1); //TODO: increment nextID
            //cout << "i+1 " << i+1 << endl;
            //cout << "nextID " << nextID << endl;
            actor.current = actions[nextID];
            return;
        }
    }

    //TODO: check which transition to take
/*    auto transition = process->getActionTransitions(sID, currentAction->getID())[0]; //first transition

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
    running = true;
}

void VRProcessEngine::pause() {
    running = false;
}

void VRProcessEngine::update() {
    if (!running) return;

    for (subject : subjects) {
        auto& actor = subject.second;
        auto& currentAction = actor.current;
        //TODO: call specific action functions for each current action

        performAction(currentAction); //dummy action function
        currentAction.duration--;
        if (currentAction.duration <= 0){
            currentAction.duration = defaultDuration;
            nextAction(actor, currentAction);
            cout << "performing action: " << currentAction.node->getLabel() << endl;
        }

    }
}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentActions(){
    vector<VRProcessNodePtr> res;

    for (subject : subjects) {
        res.push_back(subject.second.current.node);
    }

    return res;
}
