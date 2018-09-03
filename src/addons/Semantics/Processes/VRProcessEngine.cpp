#include "VRProcessEngine.h"
#include "VRProcess.h"
#include "core/utils/toString.h"
//TODO: include pthread.h

using namespace OSG;

template<> string typeName(const VRProcessEnginePtr& o) { return "ProcessEngine"; }

VRProcessEngine::VRProcessEngine() {}
VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::initialize(){
    cout << "initialize()" << endl;
    //get subject actions
    for (auto subject : process->getSubjects()){
        auto actions = process->getSubjectActions(subject->getID());
        if(!actions.size() == 0){
            subjectActions[subject->getID()] = actions;
            cout << "set current action to: " << actions[0]->getLabel() << " for subject: " << subject->getLabel() << endl;
            //activate first action initially
            currentActions[subject->getID()] = actions[0];
        }
    }
}

void VRProcessEngine::performAction(VRProcessNodePtr action){
    //signal to functions to perform current actions
    cout << "performing action: " << action->getLabel() << endl;
}

void VRProcessEngine::nextAction(int sID, VRProcessNodePtr currentAction){
    //TODO: check which transition to take
    auto transition = process->getActionTransitions(sID, currentAction->getID())[0]; //first transition

    //determine nextaction by given transition, currentAction
    auto actions = process->getTransitionActions(sID, transition->getID());
    for (auto action : actions) {
        if (action != currentAction) currentActions[sID] = action;     //update currentActions
    }
}

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; initialize();}
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {}
void VRProcessEngine::run(float speed) {
    for (auto currentAction : currentActions){
        //create a thread for each subject

        performAction(currentAction.second);
        nextAction(currentAction.first, currentAction.second);
    }
}
void VRProcessEngine::pause() {}

vector<VRProcessNodePtr> VRProcessEngine::getCurrentActions(){
    vector<VRProcessNodePtr> res;
    for (action : currentActions) {
        res.push_back(action.second);
    }
    return res;
}
