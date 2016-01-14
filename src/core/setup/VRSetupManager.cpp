#include "VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "VRSetup.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRSetupManager::VRSetupManager() {}

VRSetupManager::~VRSetupManager() {}

VRSetupPtr VRSetupManager::getCurrent() { return get()->current; }

VRSetupManager* VRSetupManager::get() {
    static VRSetupManager* mgr = new VRSetupManager();
    return mgr;
}

VRSetupPtr VRSetupManager::create() {
    current = VRSetup::create("VRSetup");
    current_path = "setup/" + current->getName() + ".xml";
    current->setScene( VRSceneManager::getCurrent() );
    return current;
}

VRSetupPtr VRSetupManager::load(string name, string path) {
    if (path == current_path) return current;
    current = VRSetup::create(name);
    current->load(path);
    current_path = path;
    current->setScene( VRSceneManager::getCurrent() );
    return current;
}

OSG_END_NAMESPACE;
