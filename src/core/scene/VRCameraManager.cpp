#include "VRCameraManager.h"
#include "../objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRCameraManager::VRCameraManager() {
    cout << "Init VRCameraManager\n";
    active=-1;
}

VRCameraManager::~VRCameraManager() {;}

VRTransform* VRCameraManager::addCamera(string name) {
    VRCamera* c = new VRCamera(name);
    int i = cameras.size();
    addCamera(c);
    setMActiveCamera(c->getName());
    return c;
}

void VRCameraManager::addCamera(VRCamera* cam) {
    int i = cameras.size();
    cameras.push_back(cam);
    cam->camID = i;
}

VRCamera* VRCameraManager::getCamera(int ID) {
    if (ID < 0 || ID > (int)cameras.size()) return 0;
    return cameras[ID];
}

void VRCameraManager::setMActiveCamera(string cam) {
    int i=0;
    for (auto c : cameras) { if (c->getName() == cam) active = i; i++; }
}

VRCamera* VRCameraManager::getActiveCamera() {
    if (active < 0 || active > (int)cameras.size()) return 0;
    return cameras[active];
}

int VRCameraManager::getActiveCameraIndex() { return active; }
vector<VRCamera*> VRCameraManager::getCameras() { return cameras; }
//vector<string> VRCameraManager::getCameraNames() { vector<string> res; for(auto c : cameras) res.push_back(c->getName()); return res; }
vector<string> VRCameraManager::getCameraNames() { vector<string> res; for(auto c : VRCamera::getAll()) res.push_back(c->getName()); return res; }

OSG_END_NAMESPACE;
