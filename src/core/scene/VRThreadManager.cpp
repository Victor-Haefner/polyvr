#include "VRThreadManager.h"
#include "core/utils/VRFunction.h"

#include <boost/thread/thread.hpp>
//#include <boost/thread.hpp>

#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGGLUT.h>


OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGThread : public ExternalThread {
    public:
        static ExternalThread* create(const char* szName, int uiId) {
            return ExternalThread::create(szName, uiId);
        }
};

void VRThreadManager::runLoop(VRThreadWeakPtr wt) {
    auto t = wt.lock();
    ExternalThreadRefPtr tr = OSGThread::create(t->name.c_str(), 0);
    tr->initialize(t->aspect);//der hauptthread nutzt Aspect 0

    t->osg_t = tr;
    t->status = 1;

    do if (t = wt.lock()) if (auto f = t->fkt.lock()) (*f)(t);
    while(t->control_flag);

    t->status = 2;
}

VRThreadManager::VRThreadManager() {
    appThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());
}

VRThreadManager::~VRThreadManager() {
    cout << "~VRThreadManager " << threads.size() << endl;
}

VRThread::VRThread() {}
VRThread::~VRThread() {
    control_flag = false;
    if (boost_t) {
        boost_t->interrupt();
        delete boost_t;
    }
}

void VRThread::syncToMain() {
    commitChanges();
    mainSyncBarrier->enter(2);
    mainSyncBarrier->enter(2);
}

void VRThread::syncFromMain() {
    if (aspect != 0) {
        selfSyncBarrier->enter(2);
        selfSyncBarrier->enter(2);

        if (initCl) {
            cout << "Sync starting thread " << initCl->getNumChanged() << " " << initCl->getNumCreated() << endl;
            initCl->applyAndClear();
        }
        appThread->getChangeList()->applyNoClear();

        selfSyncBarrier->enter(2);
        commitChangesAndClear();
    }
}

void VRThreadManager::ThreadManagerUpdate() {
    for (auto t : threads) {
        if (t.second->selfSyncBarrier->getNumWaiting() == 1) {
            t.second->selfSyncBarrier->enter(2);
            commitChanges();
            t.second->initCl->fillFromCurrentState();
            t.second->selfSyncBarrier->enter(2);
            t.second->selfSyncBarrier->enter(2);
        }

        if (t.second->mainSyncBarrier->getNumWaiting() == 1) {
            t.second->mainSyncBarrier->enter(2);
            auto cl = t.second->osg_t->getChangeList();
            cout << "Apply thread changes to main thread " << cl->getNumChanged() << " " << cl->getNumCreated() << endl;
            cl->applyAndClear();
            commitChanges();
            t.second->mainSyncBarrier->enter(2);
        }
    }
}

void VRThreadManager::stopAllThreads() {
    cout << "VRThreadManager::stopAllThreads() " << threads.size() << endl;
    for (auto t : threads) t.second->control_flag = false;

    BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
    barrier->enter();
    barrier->enter();

    int count = 0;
    while(threads.size() > 0) {
        if (count == 100) break;
        for (auto t : threads) {
            //cout << "wait for " << t.second->name << " ID " << t.second->ID << " c " << count << endl;
            if (t.second->status == 2) {
                if (t.second->boost_t) { t.second->boost_t->interrupt(); delete t.second->boost_t; t.second->boost_t = 0; }
                threads.erase(t.first); break;
            }
        }
        count++;
        osgSleep(10);
    }

    threads.clear(); // kills remaining threads!
}

void VRThreadManager::stopThread(int id, int tries) {
    if (threads.count(id) == 0) return;
    VRThreadPtr t = threads[id];
    t->control_flag = false;

    int k = 0;
    while (t->status != 2) {
        if (k == tries) { cout << "Warning: thread " << id << " won't stop" << endl; return; }
        k++;
        osgSleep(10);
    }

    if (t->boost_t) { t->boost_t->interrupt(); delete t->boost_t; t->boost_t = 0; }
    threads.erase(id);
}

int VRThreadManager::initThread(VRThreadCbPtr f, string name, bool loop, int aspect) { //start thread
    static int id = 1;

    VRThreadPtr t = VRThreadPtr( new VRThread() );
    t->aspect = aspect;
    t->control_flag = loop;
    t->appThread = appThread;
    t->name = name;
    t->ID = id;
    t->fkt = f;
    t->t_last = glutGet(GLUT_ELAPSED_TIME);
    t->selfSyncBarrier = Barrier::create();
    t->mainSyncBarrier = Barrier::create();
    t->initCl = ChangeList::create();
    t->boost_t = new boost::thread(boost::bind(&VRThreadManager::runLoop, this, t));
    threads[id] = t;

    id++;
    return t->ID;
}

void VRThreadManager::killThread(int id) {
    if (threads.count(id) == 0) return;
    cout << "\nKILL THREAD " << id << endl;
    if (threads[id]->boost_t) { threads[id]->boost_t->interrupt(); delete threads[id]->boost_t; threads[id]->boost_t = 0; }
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
