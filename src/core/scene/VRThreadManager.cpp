#include "VRThreadManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"

#include "core/utils/Thread.h"
#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>


OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGThread : public ExternalThread {
    public:
        static ExternalThread* create(const char* szName, int uiId) {
            return ExternalThread::create(szName, uiId);
        }
};

VRThread::VRThread() {}

VRThread::~VRThread() {
    control_flag = false;
    if (std_thread) {
        std_thread->join();
        delete std_thread;
    }
}

void VRThread::syncToMain() { // called in thread
    commitChanges();
    mainSyncBarrier->enter(2);
    mainSyncBarrier->enter(2);
}

void VRThread::syncFromMain() { // called in thread
    if (aspect != 0) {
        selfSyncBarrier->enter(2);
        selfSyncBarrier->enter(2);

        if (initCl) {
            //cout << "Sync starting thread, changed: " << initCl->getNumChanged() << ", created: " << initCl->getNumCreated() << endl;
            initCl->applyAndClear();
            commitChangesAndClear();
        }

        selfSyncBarrier->enter(2);
    } else {
        cout << "Warning, syncFromMain aspect is 0! -> ignore" << endl;
    }
}

VRThreadManager::VRThreadManager() {
    appThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());
    setThreadName("PolyVR_main");
}

VRThreadManager::~VRThreadManager() {
    cout << "~VRThreadManager " << threads.size() << endl;
}

void VRThreadManager::setupThreadState(VRThreadPtr t) {
    t->selfSyncBarrier->enter(2);
    t->initCl->fillFromCurrentState();
    t->initCl->merge(*appThread->getChangeList());
    //commitChanges();
    t->selfSyncBarrier->enter(2);
    t->selfSyncBarrier->enter(2);
}

void VRThreadManager::importThreadState(VRThreadPtr t) {
    t->mainSyncBarrier->enter(2);
    auto cl = t->osg_t->getChangeList();
    auto clist = Thread::getCurrentChangeList();
    clist->merge(*cl);
    cl->applyAndClear();
    t->mainSyncBarrier->enter(2);
}

void VRThreadManager::ThreadManagerUpdate() {
    for (auto t : threads) {
        if (t.second->selfSyncBarrier->getNumWaiting() == 1) setupThreadState(t.second);
        if (t.second->mainSyncBarrier->getNumWaiting() == 1) importThreadState(t.second);
    }
}

void VRThreadManager::stopAllThreads() {
#ifndef WASM
    cout << "VRThreadManager::stopAllThreads() N threads: " << threads.size() << endl;
    for (auto t : threads) t.second->control_flag = false;

    int count = 0;
    while(threads.size() > 0) {
        if (count == 100) break;
        for (auto t : threads) {
            //cout << "wait for " << t.second->name << " ID " << t.second->ID << " c " << count << endl;
            if (t.second->status == 2) {
                if (t.second->std_thread) { t.second->std_thread->join(); delete t.second->std_thread; t.second->std_thread = 0; }
                threads.erase(t.first); break;
            }
        }
        count++;
        osgSleep(10);
    }

    if (threads.size() > 0) cout << "VRThreadManager::stopAllThreads, kill " << threads.size() << " remaining threads!" << endl;
    for (auto t : threads) t.second->std_thread->detach();
    threads.clear(); // kills remaining threads!
    cout << " VRThreadManager all stopped " << endl;
#endif
}

void VRThreadManager::stopThread(int id, int tries) {
    //cout << " VRThreadManager::stopThread " << id << endl;
    if (threads.count(id) == 0) return;
    VRThreadPtr t = threads[id];
    t->control_flag = false;

    int k = 0;
    while (t->status != 2) {
        if (k == tries) { cout << "Warning: thread " << id << " won't stop" << endl; return; }
        k++;
        osgSleep(10);
    }

    if (t->std_thread) { t->std_thread->join(); delete t->std_thread; t->std_thread = 0; }
    threads.erase(id);
}

void VRThreadManager::setThreadName(string name) {
#ifdef WIN32
    std::wstring stemp = std::wstring(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), stemp.c_str());
#endif

#ifdef __linux__
    if (name.size() >= 16) name = name.substr(0, 15); // pthread_setname_np limits names to 16 chars including '\0'
    int rc = pthread_setname_np(pthread_self(), name.c_str());
    if (rc != 0) cout << "pthread_setname_np failed!!" << endl;
#endif
}

string VRThreadManager::getThreadName() {
    string name = "unknown thread";
#ifdef WIN32
    PWSTR data;
    HRESULT hr = GetThreadDescription(GetCurrentThread(), &data);
    if (SUCCEEDED(hr)) {
        std::wstring stemp = std::wstring(data);
        name = string(stemp.begin(), stemp.end());
        LocalFree(data);
    }
#endif
#ifdef __linux__
    char buf[16]; // max 16 bytes
    int rc = pthread_getname_np(pthread_self(), buf, sizeof(buf));
    if (rc == 0) name = buf;
#endif
    return name;
}

void VRThreadManager::runLoop(VRThreadWeakPtr wt) {
    auto t = wt.lock();
    if (!t) return;

    setThreadName(t->name);

    {
        ExternalThreadRefPtr tr = OSGThread::create(t->name.c_str(), 0);
        tr->initialize(t->aspect);//der hauptthread nutzt Aspect 0
        t->osg_t = tr;
    }

    t->status = 1;

    do if (t = wt.lock()) if (auto f = t->fkt.lock()) (*f)(t);
    while(t->control_flag);

    t->osg_t = 0; // avoid double free

    t->status = 2;
}

int VRThreadManager::initThread(VRThreadCbPtr f, string name, bool loop, int aspect) { //start thread
#ifndef WASM
    static int id = 1;

    VRThreadPtr t = VRThreadPtr( new VRThread() );
    t->aspect = aspect;
    t->control_flag = loop;
    t->appThread = appThread;
    t->name = name;
    t->ID = id;
    t->fkt = f;
    t->t_last = getTime()*1e-3;
    t->selfSyncBarrier = Barrier::create();
    t->mainSyncBarrier = Barrier::create();
    t->initCl = ChangeList::create();
    t->std_thread = new ::Thread(name, bind(&VRThreadManager::runLoop, this, t));
    threads[id] = t;

    id++;
    return t->ID;
#else
    cout << "VRThreadManager::initThread '" << name << "' skipped" << endl;
    return -1;
#endif
}

VRThreadPtr VRThreadManager::getThread(int id) {
    if (threads.count(id) == 0) return 0;
    return threads[id];
}

void VRThreadManager::waitThread(int id) {
    if (threads.count(id) == 0) return;
    threads[id]->std_thread->join();
}

void VRThreadManager::killThread(int id) {
    if (threads.count(id) == 0) return;
    cout << "\nKILL THREAD " << id << endl;
    if (threads[id]->std_thread) { threads[id]->std_thread->join(); delete threads[id]->std_thread; threads[id]->std_thread = 0; }
    threads.erase(id);
}

int VRThreadManager::getThreadNum() {
    return threads.size();
}

void VRThreadManager::printThreadsStats() {
    cout << "\nActive threads : " << endl;
    for (auto t : threads) cout << " Thread id : " << t.first << " , name : " << t.second->name << endl;
}

OSG_END_NAMESPACE;
