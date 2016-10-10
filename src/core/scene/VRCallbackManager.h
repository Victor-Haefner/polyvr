#ifndef VRCALLBACKMANAGER_H_INCLUDED
#define VRCALLBACKMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <list>
#include <memory>
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCallbackManager {
    private:
        struct job {
            VRUpdatePtr ptr;
            int prio = 0;
            int delay = 0;
            job() {}
            job(VRUpdatePtr j, int p, int d) : ptr(j), prio(p), delay(d) {}
        };

        struct timeoutFkt {
            VRUpdateWeakPtr fktPtr;
            int timeout;
            int last_call;
        };

        bool updateListsChanged;
        map<VRFunction<int>* , job> jobFktPtrs;
        map<int, list<timeoutFkt>* > timeoutFktPtrs;
        map<int, list<VRUpdateWeakPtr>* > updateFktPtrs;
        map<int, list<VRUpdateWeakPtr>* >::reverse_iterator fktPtr_list_itr;
        list<VRUpdateWeakPtr>::iterator fktPtr_itr;
        map<VRFunction<int>* , int> updateFktPtrs_priorities;//to easier delete functions

    public:
        VRCallbackManager();
        ~VRCallbackManager();

        void queueJob(VRUpdatePtr f, int priority = 0, int delay = 0);
        void addUpdateFkt(VRUpdateWeakPtr f, int priority = 0);
        void addTimeoutFkt(VRUpdateWeakPtr f, int priority, int timeout);

        void dropUpdateFkt(VRUpdateWeakPtr f);
        void dropTimeoutFkt(VRUpdateWeakPtr f);

        void clearJobs();
        void updateCallbacks();
        void printCallbacks();
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKMANAGER_H_INCLUDED
