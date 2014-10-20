#ifndef VRCALLBACKMANAGER_H_INCLUDED
#define VRCALLBACKMANAGER_H_INCLUDED

#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCallbackManager {
    private:
        int id;//one single counter for all signals and callbacks

        //functions maps
        map<int, VRFunction_base*> callbacks;
        map<int, VRSignal*> signals;
        list<int*> bonds;//signal/callback pairs
        map<int, string> labels;//maps callback and signal to their names

        int id_increment(string name);

        int findByName(string name);

        string fromInt(int i);

        string getLinkLabel(unsigned int i);

    public:
        VRCallbackManager();
        int addCallback(VRFunction_base* fkt, string name = "");

        int addSignal( scene, VRSignal* sig, string name = "");

        VRFunction_base* getCallback(string name);

        void splitLink(int sig, int cb);
        void splitLink(VRSignal* sig, VRFunction_base* fkt);

        //main bond function
        bool bindLink(int id_sig, int id_cb);
        bool bindLink(VRFunction_base* fkt, VRSignal* sig, string fkt_name, string sig_name);
        bool bindLink(VRSignal* sig, VRFunction_base* fkt, string sig_name, string fkt_name);

        int getNumCallbacks();
        int getNumSignals();
        int getNumLinks();

        int getLinkSignal(unsigned int i);
        int getLinkCallback(unsigned int i);

        string getSignalLabel(unsigned int i);

        string getCallbackLabel(unsigned int i);
};

OSG_END_NAMESPACE;

#endif // VRCALLBACKMANAGER_H_INCLUDED
