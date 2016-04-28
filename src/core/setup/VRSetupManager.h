#ifndef VRSETUPMANAGER_H_INCLUDED
#define VRSETUPMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>

#include "core/setup/VRSetupFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSetupManager {
    private:
        VRSetupPtr current = 0;
        string current_path;

        VRSetupManager();

    public:
        static VRSetupManager* get();
        static VRSetupPtr getCurrent();
        ~VRSetupManager();

        void closeSetup();
        VRSetupPtr create();
        VRSetupPtr load(string name, string path);
};

OSG_END_NAMESPACE;

#endif // VRSETUPMANAGER_H_INCLUDED
