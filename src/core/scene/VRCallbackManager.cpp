#include "VRCallbackManager.h"
#include "core/utils/VRFunction.h"
#include "core/objects/object/VRObject.h"
#include <iostream>
#include <vector>
#include <GL/glut.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRCallbackManager::VRCallbackManager() { updateListsChanged = false; }
VRCallbackManager::~VRCallbackManager() {
    map<int, list<VRFunction<int>*>*>::iterator itr1;
    list<VRFunction<int>*>::iterator itr2;

    for (auto ufs : updateFkts) {
        for (auto uf : *ufs.second) delete uf;
        delete ufs.second;
    }
}

void VRCallbackManager::queueJob(VRFunction<int>* f, int priority) {
    updateListsChanged = true;
    jobFkts[f] = priority;
    addUpdateFkt(f, priority);
}

void VRCallbackManager::queueEvent(VRFunction<int>* f, float delay, int priority) {
    /*updateListsChanged = true;
    jobFkts[f] = priority;
    addUpdateFkt(f, priority);*/ // TODO
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
    tof.last_call = glutGet(GLUT_ELAPSED_TIME);
    timeoutFkts[priority]->push_back(tof);
}

void VRCallbackManager::dropUpdateFkt(VRFunction<int>* f) {//replace by list || map || something..
    if (updateFkts_priorities.count(f) == 0) return;
    int prio = updateFkts_priorities[f];
    list<VRFunction<int>*>* l = updateFkts[prio];
    if (l == 0) return;

    l->remove(f);
    updateFkts_priorities.erase(f);
}

void VRCallbackManager::dropTimeoutFkt(VRFunction<int>* f) {//replace by list || map || something..
    updateListsChanged = true;
    if (updateFkts_priorities.count(f) == 0) return;
    int prio = updateFkts_priorities[f];
    if (timeoutFkts.count(prio) == 0) return;
    list<timeoutFkt>* l = timeoutFkts[prio];
    if (l == 0) return;

    //for (auto fkt : *l) if(fkt->fkt == f) { l->erase(fkt); break; }

    list<timeoutFkt>::iterator itr;
    for (itr = l->begin(); itr != l->end(); itr++)
        if (itr->fkt == f) { l->erase(itr); break; }

    updateFkts_priorities.erase(f);
}

void VRCallbackManager::updateCallbacks() {
    //printCallbacks();
    vector<VRFunction<int>*> cbs;

    // gather all update callbacks
    for (auto fl : updateFkts) for (auto f : *fl.second) cbs.push_back(f);

    // gather all timeout callbacks
    int time = glutGet(GLUT_ELAPSED_TIME);
    for (auto tfl : timeoutFkts)
        for (auto& tf : *tfl.second)
            if (time - tf.last_call >= tf.timeout) {
                cbs.push_back(tf.fkt);
                tf.last_call = time;
            }

    for (auto cb : cbs) { // trigger all callbacks
        VRFunction<int>& rcb = *cb;
        rcb(0);
        //(*cb)(0);
        if (jobFkts.count(cb)) { // if a job erase it
            dropUpdateFkt(cb);
            jobFkts.erase(cb);
        }
    }
}

void VRCallbackManager::printCallbacks() {
    cout << "VRCallbackManager " << this << " t " << VRGlobals::get()->CURRENT_FRAME << endl;
    cout << " update fkts (" << updateFkts.size() << ")\n";
    for (auto fl : updateFkts) {
        cout << "  prio " << fl.first << " (" << fl.second->size() << ")\n";
        for (auto f : *fl.second) {
            cout << "   fkt " << f->getName() << endl;
        }
    }
}

OSG_END_NAMESPACE
