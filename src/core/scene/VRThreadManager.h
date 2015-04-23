#ifndef VRTHREADMANAGER_H_INCLUDED
#define VRTHREADMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>

#include <OpenSG/OSGThread.h> // TODO: find out how to forward declare ref ptr
#include <OpenSG/OSGThreadManager.h>

namespace boost{ class thread; }
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class ExternalThread;

class VRThread {
    public:
        ~VRThread();

        ThreadRefPtr appThread;
        int ID = -1;
        boost::thread* boost_t = 0;
        ExternalThreadRefPtr osg_t = 0;
        BarrierRefPtr barrier;
        string name;
        bool control_flag = false;
        int status = 0;
        VRFunction<VRThread*>* fkt = 0;
        int aspect = 0;
        /** last frame time stamp**/
        long long t_last = 0;

};

class VRThreadManager {
    private:
        ThreadRefPtr appThread;
        map<int, VRThread*> threads;

        void runLoop(VRThread* t);

    public:
        VRThreadManager();
        ~VRThreadManager();

        int initThread(VRFunction<VRThread*>* f, string name, bool loop = false, int aspect = 0);


        void stopThread(int id, int tries = 100);
        void killThread(int id);

        int getThreadNum();

        void printThreadsStats();

    protected:

        void ThreadManagerUpdate();
};

OSG_END_NAMESPACE;

#endif // VRTHREADMANAGER_H_INCLUDED
