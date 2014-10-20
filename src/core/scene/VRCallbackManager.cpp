#include "VRCallbackManager.h"
#include <SDL/SDL.h>
#include "core/utils/VRFunction.h"
#include <iostream>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRCallbackManager::VRCallbackManager() { updateListsChanged = false; }
VRCallbackManager::~VRCallbackManager() {
    map<int, list<VRFunction<int>*>*>::iterator itr1;
    list<VRFunction<int>*>::iterator itr2;

    for (itr1 = updateFkts.begin(); itr1 != updateFkts.end(); itr1++) {
        for (itr2 = itr1->second->begin(); itr2 != itr1->second->end(); itr2++) {
            delete *itr2;
        }
        delete itr1->second;
    }
}

void VRCallbackManager::queueJob(VRFunction<int>* f, int priority) {
    updateListsChanged = true;
    jobFkts[f] = priority;
    addUpdateFkt(f, priority);
}

void VRCallbackManager::addUpdateFkt(VRFunction<int>* f, int priority) {
    updateListsChanged = true;
    if (updateFkts.count(priority) == 0) {
        updateFkts[priority] = new list<VRFunction<int>*>();
    }
    updateFkts_priorities[f] = priority;
    updateFkts[priority]->push_back(f);
}

void VRCallbackManager::addTimeoutFkt(VRFunction<int>* f, int priority, int timeout) {
    updateListsChanged = true;
    if (updateFkts_priorities.count(f)) return;

    if (timeoutFkts.count(priority) == 0) timeoutFkts[priority] = new list<timeoutFkt>();
    updateFkts_priorities[f] = priority;

    timeoutFkt tof;
    tof.fkt = f;
    tof.timeout = timeout;
    tof.last_call = SDL_GetTicks();
    timeoutFkts[priority]->push_back(tof);
}

void VRCallbackManager::dropUpdateFkt(VRFunction<int>* f) {//replace by list or map or something..
    if (updateFkts_priorities.count(f) == 0) return;
    int prio = updateFkts_priorities[f];
    list<VRFunction<int>*>* l = updateFkts[prio];
    l->remove(f);
    updateFkts_priorities.erase(f);
}

void VRCallbackManager::dropTimeoutFkt(VRFunction<int>* f) {//replace by list or map or something..
    updateListsChanged = true;
    if (updateFkts_priorities.count(f) == 0) return;
    int prio = updateFkts_priorities[f];
    if (timeoutFkts.count(prio) == 0) return;
    list<timeoutFkt>* l = timeoutFkts[prio];
    if (l == 0) return;

    list<timeoutFkt>::iterator itr;
    for (itr = l->begin(); itr != l->end(); itr++)
        if (itr->fkt == f) { l->erase(itr); break; }

    updateFkts_priorities.erase(f);
}

void VRCallbackManager::updateCallbacks() {
    updateListsChanged = false;
    map<int, list<timeoutFkt>* >::reverse_iterator itr1;
    list<timeoutFkt>::iterator itr2;
    for (itr1 = timeoutFkts.rbegin(); itr1 != timeoutFkts.rend(); itr1++) {
        list<timeoutFkt>* l = itr1->second;
        for (itr2 = l->begin(); itr2 != l->end(); itr2++) {
            if (updateListsChanged) break;
            VRFunction<int>* f = itr2->fkt;

            int time = SDL_GetTicks();
            if (time - itr2->last_call >= itr2->timeout) {
                (*f)(0);
                itr2->last_call = time;
            }
        }
    }

    for (fkt_list_itr = updateFkts.rbegin(); fkt_list_itr != updateFkts.rend(); fkt_list_itr++) {
        list<VRFunction<int>*> l = *fkt_list_itr->second;
        //cout << "\nPrio: " << fkt_list_itr->first << " " << fkt_list_itr->second;
        for (fkt_itr = l.begin(); fkt_itr != l.end(); fkt_itr++) {
            VRFunction<int>* f = *fkt_itr;
            (*f)(0);

            if (jobFkts.count(f)) { // if a job erase it
                dropUpdateFkt(f);
                jobFkts.erase(f);
            }

            if (updateListsChanged) break;
        }
    }
}


OSG_END_NAMESPACE
