#ifndef VRTHREADMANAGER_H_INCLUDED
#define VRTHREADMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <memory>

#include <OpenSG/OSGThread.h> // TODO: find out how to forward declare ref ptr
#include <OpenSG/OSGThreadManager.h>

#include "core/utils/VRFunctionFwd.h"

namespace boost{ class thread; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class ExternalThread;

class VRThread {
    public:
        VRThread();
        ~VRThread();

        ThreadRefPtr appThread;
        int ID = -1;
        boost::thread* boost_t = 0;
        ExternalThreadRefPtr osg_t = 0;
        BarrierRefPtr barrier;
        string name;
        bool control_flag = false;
        int status = 0;
        VRFunction< std::weak_ptr<VRThread> >* fkt = 0;
        int aspect = 0;
        /** last frame time stamp**/
        long long t_last = 0;
};

typedef std::shared_ptr<VRThread> VRThreadPtr;
typedef std::weak_ptr<VRThread> VRThreadWeakPtr;

class VRThreadManager {
    private:
        ThreadRefPtr appThread;
        map<int, VRThreadPtr> threads;

        void runLoop(VRThreadWeakPtr t);

    public:
        VRThreadManager();
        ~VRThreadManager();

        int initThread(VRFunction<VRThreadWeakPtr>* f, string name, bool loop = false, int aspect = 0);

        void stopThread(int id, int tries = 100);
        void stopAllThreads();
        void killThread(int id);

        int getThreadNum();

        void printThreadsStats();

    protected:

        void ThreadManagerUpdate();
};

OSG_END_NAMESPACE;

#endif // VRTHREADMANAGER_H_INCLUDED
