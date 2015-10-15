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
    for (auto ufs : updateFktPtrs) delete ufs.second;
}

void VRCallbackManager::queueJob(VRUpdatePtr f, int priority) {
    updateListsChanged = true;
    jobFktPtrs[f.get()] = job(f,priority);
}

void VRCallbackManager::addUpdateFkt(VRUpdateWeakPtr f, int priority) {
    updateListsChanged = true;
    if (updateFktPtrs.count(priority) == 0) {
        updateFktPtrs[priority] = new list<VRUpdateWeakPtr>();
    }
    updateFktPtrs_priorities[f.lock().get()] = priority;
    updateFktPtrs[priority]->push_back(f);
}

void VRCallbackManager::addTimeoutFkt(VRUpdateWeakPtr p, int priority, int timeout) {
    auto f = p.lock().get();
    updateListsChanged = true;
    if (updateFktPtrs_priorities.count(f)) return;

    if (timeoutFktPtrs.count(priority) == 0) timeoutFktPtrs[priority] = new list<timeoutFkt>();
    updateFktPtrs_priorities[f] = priority;

    timeoutFkt tof;
    tof.fktPtr = p;
    tof.timeout = timeout;
    tof.last_call = glutGet(GLUT_ELAPSED_TIME);
    timeoutFktPtrs[priority]->push_back(tof);
}

void VRCallbackManager::dropUpdateFkt(VRUpdateWeakPtr p) {//replace by list || map || something..
    auto f = p.lock().get();
    if (updateFktPtrs_priorities.count(f) == 0) return;
    int prio = updateFktPtrs_priorities[f];
    list<VRUpdateWeakPtr>* l = updateFktPtrs[prio];
    if (l == 0) return;

    l->remove_if([p](VRUpdateWeakPtr p2){
        auto sp = p.lock();
        auto sp2 = p2.lock();
        return (sp && sp2) ? sp == sp2 : false;
    });

    updateFktPtrs_priorities.erase(f);
}

void VRCallbackManager::dropTimeoutFkt(VRUpdateWeakPtr p) {//replace by list || map || something..
    auto f = p.lock().get();
    updateListsChanged = true;
    if (updateFktPtrs_priorities.count(f) == 0) return;
    int prio = updateFktPtrs_priorities[f];
    if (timeoutFktPtrs.count(prio) == 0) return;
    list<timeoutFkt>* l = timeoutFktPtrs[prio];
    if (l == 0) return;

    auto sp = p.lock();
    list<timeoutFkt>::iterator itr;
    for (itr = l->begin(); itr != l->end(); itr++) {
        auto sp2 = itr->fktPtr.lock();
        if( (sp && sp2) ? sp == sp2 : false ) { l->erase(itr); break; }
    }

    updateFktPtrs_priorities.erase(f);
}

void VRCallbackManager::updateCallbacks() {
    //printCallbacks();
    vector<VRUpdateWeakPtr> cbsPtr;

    // gather all update callbacks
    for (auto fl : updateFktPtrs) for (auto f : *fl.second) cbsPtr.push_back(f);

    // gather all timeout callbacks
    int time = glutGet(GLUT_ELAPSED_TIME);
    for (auto tfl : timeoutFktPtrs)
        for (auto& tf : *tfl.second)
            if (time - tf.last_call >= tf.timeout) {
                cbsPtr.push_back(tf.fktPtr);
                tf.last_call = time;
            }

    for (auto cb : cbsPtr) { // trigger all callbacks
        if ( auto scb = cb.lock()) (*scb)(0);
    }

    for (auto j : jobFktPtrs) {
        if (j.second.ptr) {
            string name = j.second.ptr->getName();
            (*j.second.ptr)(0);
        }
    }
    jobFktPtrs.clear();
}

void VRCallbackManager::printCallbacks() {
    cout << "VRCallbackManager " << this << " t " << VRGlobals::get()->CURRENT_FRAME << endl;
    cout << " update fkts (" << updateFktPtrs.size() << ")\n";
    for (auto fl : updateFktPtrs) {
        cout << "  prio " << fl.first << " (" << fl.second->size() << ")\n";
        for (auto f : *fl.second) {
            auto sp = f.lock();
            if (sp) cout << "   fkt " << sp->getName() << endl;
        }
    }
}

OSG_END_NAMESPACE
