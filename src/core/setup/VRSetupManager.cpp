#include "VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "VRSetup.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRSetupManager::VRSetupManager() {
    current = 0;
}

VRSetupManager::~VRSetupManager() {
    delete current;
}

VRSetupManager* VRSetupManager::get() {
    static VRSetupManager* mgr = new VRSetupManager();
    return mgr;
}

VRSetup* VRSetupManager::create() {
    if (current) delete current;
    current = new VRSetup("VRSetup");
    current_path = "setup/" + current->getName() + ".xml";
    current->setScene( VRSceneManager::getCurrent() );
    return current;
}

VRSetup* VRSetupManager::load(string name, string path) {
    if (path == current_path) return current;
    if (current) delete current;

    current = new VRSetup(name);
    current->load(path);
    current_path = path;
    current->setScene( VRSceneManager::getCurrent() );
    return current;
}

VRSetup* VRSetupManager::getCurrent() { return get()->current; }

OSG_END_NAMESPACE;
