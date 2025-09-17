#include "VRCallbackManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"
#include "core/objects/object/VRObject.h"
#include <iostream>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRMutex mtx;

VRCallbackManager::VRCallbackManager() { updateListsChanged = false; }
VRCallbackManager::~VRCallbackManager() {
    cout << "VRCallbackManager::~VRCallbackManager" << endl;
}

void VRCallbackManager::queueJob(VRUpdateCbPtr f, int priority, int delay, bool ownRef) {
    //cout << "VRCallbackManager::queueJob " << f->name << ", ownRef: " << ownRef << endl;
    VRLock lock(mtx);
    updateListsChanged = true;
    VRUpdateCbWeakPtr w = f;
    if (ownRef) jobFktPtrs.push_back( job(f,priority,delay) );
    else        jobFktPtrs.push_back( job(w,priority,delay) );
}

void VRCallbackManager::addUpdateFkt(VRUpdateCbWeakPtr f, int priority) {
    VRLock lock(mtx);
    updateListsChanged = true;
    if (updateFktPtrs.count(priority) == 0) {
        updateFktPtrs[priority] = shared_ptr<list<VRUpdateCbWeakPtr>>(new list<VRUpdateCbWeakPtr>());
    }
    updateFktPtrs_priorities[f.lock().get()] = priority;
    updateFktPtrs[priority]->push_back(f);
}

void VRCallbackManager::addTimeoutFkt(VRUpdateCbWeakPtr p, int priority, int timeout) {
    VRLock lock(mtx);
    auto f = p.lock().get();
    updateListsChanged = true;
    if (updateFktPtrs_priorities.count(f)) return;

    if (timeoutFktPtrs.count(priority) == 0) timeoutFktPtrs[priority] = shared_ptr<list<timeoutFkt>>(new list<timeoutFkt>());
    updateFktPtrs_priorities[f] = priority;

    timeoutFkt tof;
    tof.fktPtr = p;
    tof.timeout = timeout;
    tof.last_call = getTime()*1e-3;
    timeoutFktPtrs[priority]->push_back(tof);
}

void VRCallbackManager::dropUpdateFkt(VRUpdateCbWeakPtr p) {//replace by list || map || something..
    VRLock lock(mtx);
    auto f = p.lock().get();
    if (updateFktPtrs_priorities.count(f) == 0) return;
    int prio = updateFktPtrs_priorities[f];
    if (!updateFktPtrs.count(prio)) return;
    auto l = updateFktPtrs[prio];
    if (!l) return;

    l->remove_if([p](VRUpdateCbWeakPtr p2){
        auto sp = p.lock();
        auto sp2 = p2.lock();
        return (sp && sp2) ? sp == sp2 : false;
    });

    updateFktPtrs_priorities.erase(f);
}

void VRCallbackManager::dropTimeoutFkt(VRUpdateCbWeakPtr p) {//replace by list || map || something..
    VRLock lock(mtx);
    auto f = p.lock().get();
    updateListsChanged = true;
    if (updateFktPtrs_priorities.count(f) == 0) return;
    int prio = updateFktPtrs_priorities[f];
    if (timeoutFktPtrs.count(prio) == 0) return;
    auto l = timeoutFktPtrs[prio];
    if (!l) return;

    auto sp = p.lock();
    list<timeoutFkt>::iterator itr;
    for (itr = l->begin(); itr != l->end(); itr++) {
        auto sp2 = itr->fktPtr.lock();
        if( (sp && sp2) ? sp == sp2 : false ) { l->erase(itr); break; }
    }

    updateFktPtrs_priorities.erase(f);
}

void VRCallbackManager::updateCallbacks() {
    VRLock lock(mtx);
    //printCallbacks();
    vector<VRUpdateCbWeakPtr> cbsPtr;

    // gather all timeout callbacks, TODO: use priority system!
    int time = getTime()*1e-3;
    for (auto tfl : timeoutFktPtrs)
        for (auto& tf : *tfl.second)
            if (time - tf.last_call >= tf.timeout) {
                cbsPtr.push_back(tf.fktPtr);
                tf.last_call = time;
            }

    // gather all update callbacks
    for (auto fl : updateFktPtrs) for (auto f : *fl.second) cbsPtr.push_back(f);

    for (auto cb : cbsPtr) { // trigger all callbacks
        if (auto scb = cb.lock()) (*scb)();
    }

    vector<job> delayedJobs;
    vector<job> toExecuteJobs;
    for (auto j : jobFktPtrs) {
        if (j.delay > 0) {
            j.delay--;
            delayedJobs.push_back(j);
        } else toExecuteJobs.push_back(j);
    }
    jobFktPtrs = delayedJobs;

    for (auto j : toExecuteJobs) {
        if (!j.ptr) j.ptr = j.wptr.lock();
        if (j.ptr) (*j.ptr)();
    }
}

void VRCallbackManager::printCallbacks() {
    VRLock lock(mtx);
    cout << "VRCallbackManager " << this << " t " << VRGlobals::CURRENT_FRAME << endl;
    cout << " update fkts (" << updateFktPtrs.size() << ")\n";
    for (auto fl : updateFktPtrs) {
        cout << "  prio " << fl.first << " (" << fl.second->size() << ")\n";
        for (auto f : *fl.second) {
            auto sp = f.lock();
            if (sp) cout << "   fkt " << sp->name << endl;
        }
    }
}

OSG_END_NAMESPACE
