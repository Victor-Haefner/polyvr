#include "VRCameraManager.h"
#include "../objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRCameraManager::VRCameraManager() {
    cout << "Init VRCameraManager\n";
    active=-1;
}

VRCameraManager::~VRCameraManager() {
    // cameras get destroyed from the tree
    //for (int i=0; i<cameras.size(); i++) delete cameras[i];
}

VRTransform* VRCameraManager::addCamera(string name) {
    VRCamera* c = new VRCamera(name);
    cameras.push_back(c);
    setActiveCamera(cameras.size()-1);

    return c;
}

void VRCameraManager::addCamera(VRCamera* cam) {
    cameras.push_back(cam);
}

VRCamera* VRCameraManager::getCamera(int ID) {
    if (ID < 0 || ID > (int)cameras.size()) return 0;
    return cameras[ID];
}

void VRCameraManager::setActiveCamera(int ID) {
    if (ID < 0 || ID > (int)cameras.size()) return;
    active = ID;
}

VRCamera* VRCameraManager::getActiveCamera() {
    if (active < 0 || active > (int)cameras.size()) return 0;
    return cameras[active];
}

int VRCameraManager::getActiveCameraIndex() { return active; }
vector<VRCamera*> VRCameraManager::getCameras() { return cameras; }
vector<string> VRCameraManager::getCameraNames() { vector<string> res; for(auto c : cameras) res.push_back(c->getName()); return res; }

OSG_END_NAMESPACE;
