#include "VRCameraManager.h"
#include "VRSpaceWarper.h"
#include "../objects/VRCamera.h"
#include "core/utils/VRFunction.h"
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRCameraManager::VRCameraManager() {
    cout << "Init VRCameraManager\n";
    //setupSpaceWarper(true);
    setStorageType("Cameras");
    store("activeCam", &activeName);
}

VRCameraManager::~VRCameraManager() {;}

void VRCameraManager::CMsetup() {
    setMActiveCamera(activeName);
}

VRCameraPtr VRCameraManager::getCamera(int ID) {
    int i=0;
    for (auto c : VRCamera::getAll()) { if (i == ID) return c.lock(); i++; }
    return 0;
}

void VRCameraManager::setMActiveCamera(string cam) {
    for (auto c : VRCamera::getAll()) {
        if (auto sp = c.lock()) if (sp->getName() == cam) { active = sp; activeName = cam; }
    }
}

VRCameraPtr VRCameraManager::getActiveCamera() {
    return active.lock();
}

int VRCameraManager::getActiveCameraIndex() {
    int index = -1;
    if (active.lock()) {
        int i=0;
        for (auto c : VRCamera::getAll()) {
            if (c.lock() == active.lock()) { index = i; break; }
            i++;
        }
    }
    return index;
}

vector<string> VRCameraManager::getCameraNames() {
    vector<string> res;
    for(auto c : VRCamera::getAll()) { if (auto sp = c.lock()) res.push_back(sp->getName()); }
    return res;
}

void VRCameraManager::setupSpaceWarper(bool b) {
    if (!spaceWarper && b) {
        spaceWarper = VRSpaceWarper::create();
    }
}

VRSpaceWarperPtr VRCameraManager::getSpaceWarper() { return spaceWarper; }

OSG_END_NAMESPACE;
