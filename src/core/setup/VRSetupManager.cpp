#include "VRSetupManager.h"
#include "VRSetup.h"
#include "core/scene/VRScene.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRSetupManager* setup_mgr = 0;

VRSetupManager::VRSetupManager() {
    setup_mgr = this;
}

VRSetupManager::~VRSetupManager() {
    cout << "VRSetupManager::~VRSetupManager" << endl;
    setup_mgr = 0;
}

VRSetupManagerPtr VRSetupManager::create() { return VRSetupManagerPtr(new VRSetupManager()); }
VRSetupManager* VRSetupManager::get() { return setup_mgr; }
VRSetupPtr VRSetupManager::getCurrent() { return get()->current; }
void VRSetupManager::closeSetup() { current = 0; }

VRSetupPtr VRSetupManager::newSetup() {
    current = VRSetup::create("VRSetup");
    current_path = "setup/" + current->getName() + ".xml";
    current->setScene( VRScene::getCurrent() );
    return current;
}

VRSetupPtr VRSetupManager::load(string name, string path) {
    if (path == current_path) return current;
    current = VRSetup::create(name);
    current->load(path);
    current_path = path;
    current->setScene( VRScene::getCurrent() );
    return current;
}

OSG_END_NAMESPACE;
