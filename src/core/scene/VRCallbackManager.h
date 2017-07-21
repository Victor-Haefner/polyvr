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
            VRUpdateCbPtr ptr;
            int prio = 0;
            int delay = 0;
            job() {}
            job(VRUpdateCbPtr j, int p, int d) : ptr(j), prio(p), delay(d) {}
        };

        struct timeoutFkt {
            VRUpdateCbWeakPtr fktPtr;
            int timeout;
            int last_call;
        };

        bool updateListsChanged;
        map<VRUpdateCb*, job> jobFktPtrs;
        map<int, list<timeoutFkt>* > timeoutFktPtrs;
        map<int, list<VRUpdateCbWeakPtr>* > updateFktPtrs;
        map<int, list<VRUpdateCbWeakPtr>* >::reverse_iterator fktPtr_list_itr;
        list<VRUpdateCbWeakPtr>::iterator fktPtr_itr;
        map<VRUpdateCb*, int> updateFktPtrs_priorities;//to easier delete functions

    public:
        VRCallbackManager();
        ~VRCallbackManager();

        void queueJob(VRUpdateCbPtr f, int priority = 0, int delay = 0);
        void addUpdateFkt(VRUpdateCbWeakPtr f, int priority = 0);
        void addTimeoutFkt(VRUpdateCbWeakPtr f, int priority, int timeout);

        void dropUpdateFkt(VRUpdateCbWeakPtr f);
        void dropTimeoutFkt(VRUpdateCbWeakPtr f);

        void clearJobs();
        void updateCallbacks();
        void printCallbacks();
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKMANAGER_H_INCLUDED
