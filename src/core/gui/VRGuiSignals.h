#ifndef VRGUISIGNALS_H_INCLUDED
#define VRGUISIGNALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRDeviceFwd.h"
#include <string>
#include <vector>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSignals {
    private:
        map<string, VRSignalPtr> signals;

        VRGuiSignals();

    public:
        static VRGuiSignals* get();

        vector<string> getSignals();
        VRSignalPtr getSignal(string name);

        void clear();
};

OSG_END_NAMESPACE

#endif // VRGUISIGNALS_H_INCLUDED
