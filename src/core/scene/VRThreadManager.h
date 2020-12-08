#ifndef VRTHREADMANAGER_H_INCLUDED
#define VRTHREADMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <memory>

#include <OpenSG/OSGThread.h> // TODO: find out how to forward declare ref ptr
#include <OpenSG/OSGThreadManager.h>

#include "core/utils/VRFunctionFwd.h"

#ifdef WASM
namespace std{ inline namespace __2{ class thread; }; }
#else
namespace std{ class thread; }
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

class ExternalThread;
class ChangeList;

class VRThread {
    public:
        VRThread();
        ~VRThread();

        ThreadRefPtr appThread;
        int ID = -1;
        std::thread* std_thread = 0;
        ExternalThreadRefPtr osg_t = 0;
        BarrierRefPtr mainSyncBarrier;
        BarrierRefPtr selfSyncBarrier;
        ChangeList* initCl = 0;
        string name;
        bool control_flag = false;
        int status = 0;
        VRThreadCbWeakPtr fkt;
        int aspect = 0;
        /** last frame time stamp**/
        long long t_last = 0;

        void syncFromMain();
        void syncToMain();
};

class VRThreadManager {
    private:
        ThreadRefPtr appThread;
        map<int, VRThreadPtr> threads;

        void runLoop(VRThreadWeakPtr t);

    public:
        VRThreadManager();
        virtual ~VRThreadManager();

        int initThread(VRThreadCbPtr f, string name, bool loop = false, int aspect = 0);
        VRThreadPtr getThread(int id);

        void waitThread(int id);
        void stopThread(int id, int tries = 100);
        void stopAllThreads();
        void killThread(int id);

        int getThreadNum();

        void printThreadsStats();

        void setupThreadState(VRThreadPtr t);
        void importThreadState(VRThreadPtr t);
        void ThreadManagerUpdate();
};

OSG_END_NAMESPACE;

#endif // VRTHREADMANAGER_H_INCLUDED
