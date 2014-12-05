#ifndef VRCALLBACKMANAGER_H_INCLUDED
#define VRCALLBACKMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <list>

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCallbackManager {
    private:
        struct timeoutFkt {
            VRFunction<int>* fkt;
            int timeout;
            int last_call;
        };

        bool updateListsChanged;
        map<VRFunction<int>* , int> jobFkts;
        map<int, list<timeoutFkt>* > timeoutFkts;
        map<int, list<VRFunction<int>*>* > updateFkts;
        map<int, list<VRFunction<int>*>* >::reverse_iterator fkt_list_itr;
        list<VRFunction<int>*>::iterator fkt_itr;
        map<VRFunction<int>* , int> updateFkts_priorities;//to easier delete functions

    public:
        VRCallbackManager();
        ~VRCallbackManager();

        void queueJob(VRFunction<int>* f, int priority = 0);
        void queueEvent(VRFunction<int>* f, float delay = 0, int priority = 0);

        void addUpdateFkt(VRFunction<int>* f, int priority = 0);
        void addTimeoutFkt(VRFunction<int>* f, int priority, int timeout);

        void dropUpdateFkt(VRFunction<int>* f);
        void dropTimeoutFkt(VRFunction<int>* f);

        void clearJobs();

        void updateCallbacks();
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKMANAGER_H_INCLUDED
