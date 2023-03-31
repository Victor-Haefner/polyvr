#ifndef VRGUISIGNALS_H_INCLUDED
#define VRGUISIGNALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRDeviceFwd.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

#define uiSignal(m,...) OSG::VRGuiManager::trigger(m,##__VA_ARGS__)

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSignals {
    public:
        typedef map<string, string> Options;
        typedef function< bool(Options) > Callback;
        typedef function< bool(int,int,int,int) > ResizeCallback;

    private:
        map<string, VRSignalPtr> signals;
        map<string, vector<Callback>> callbacks;
        map<string, vector<ResizeCallback>> resizeCallbacks;

        VRGuiSignals();

    public:
        static VRGuiSignals* get();

        vector<string> getSignals();
        VRSignalPtr getSignal(string name);

        void addCallback(string name, Callback callback);
        void addResizeCallback(string name, ResizeCallback callback);
        bool trigger(string name, Options options = {});
        bool triggerResize(string name, int,int,int,int);

        void clear();
};

OSG_END_NAMESPACE

#endif // VRGUISIGNALS_H_INCLUDED
