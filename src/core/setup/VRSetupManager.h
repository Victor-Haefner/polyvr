#ifndef VRSETUPMANAGER_H_INCLUDED
#define VRSETUPMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSetup;

class VRSetupManager {
    private:
        VRSetup* current;
        string current_path;

        VRSetupManager();

    public:
        static VRSetupManager* get();
        ~VRSetupManager();

        VRSetup* create();
        VRSetup* load(string name, string path);

        VRSetup* getCurrent();
};

OSG_END_NAMESPACE;

#endif // VRSETUPMANAGER_H_INCLUDED
