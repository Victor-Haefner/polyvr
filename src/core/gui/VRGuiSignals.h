#ifndef VRGUISIGNALS_H_INCLUDED
#define VRGUISIGNALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/devices/VRSignal.h"
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSignals {
    private:
        map<string, VRSignal*> signals;

        VRGuiSignals();

    public:
        static VRGuiSignals* get();

        VRSignal* getSignal(string name);
};

OSG_END_NAMESPACE

#endif // VRGUISIGNALS_H_INCLUDED
