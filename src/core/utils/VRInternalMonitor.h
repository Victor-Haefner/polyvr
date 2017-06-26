#ifndef VRINTERNALMONITOR_H_INCLUDED
#define VRINTERNALMONITOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>

#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRInternalMonitor {
    private:
        typedef VRFunction<string&> varFkt;
        map<string, varFkt*> varFkts;
        VRInternalMonitor();
        bool doUpdate = false;

    public:
        ~VRInternalMonitor();
        static VRInternalMonitor* get();

        void add( string name, varFkt* fkt );

        void update();

        map<string, string> getVariables();
};

OSG_END_NAMESPACE;

#endif // VRINTERNALMONITOR_H_INCLUDED
