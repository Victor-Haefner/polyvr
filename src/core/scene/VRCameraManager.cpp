#include "VRCameraManager.h"
#include "../objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRCameraManager::VRCameraManager() {
    cout << "Init VRCameraManager\n";
}

VRCameraManager::~VRCameraManager() {;}

VRTransform* VRCameraManager::addCamera(string name) {
    VRCamera* c = new VRCamera(name);
    setMActiveCamera(c->getName());
    return c;
}

VRCamera* VRCameraManager::getCamera(int ID) {
    int i=0;
    for (auto c : VRCamera::getAll()) { if (i == ID) return c; i++; }
    return 0;
}

void VRCameraManager::setMActiveCamera(string cam) {
    for (auto c : VRCamera::getAll()) { if (c->getName() == cam) active = c; }
}

VRCamera* VRCameraManager::getActiveCamera() {
    return active;
}

int VRCameraManager::getActiveCameraIndex() {
    int i=0;
    for (auto c : VRCamera::getAll()) { if (c == active) return i; i++; }
    return -1;
}

vector<string> VRCameraManager::getCameraNames() { vector<string> res; for(auto c : VRCamera::getAll()) res.push_back(c->getName()); return res; }

OSG_END_NAMESPACE;
