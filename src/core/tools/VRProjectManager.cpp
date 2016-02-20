#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"

using namespace OSG;

VRProjectManager::VRProjectManager() : VRSprite("ProjectManager") { initSite(); }

void VRProjectManager::initSite() {
    ;
}

void VRProjectManager::addStorage(VRStoragePtr s) { vault.push_back(s); }

void VRProjectManager::store(string path) {
    ;
}

void VRProjectManager::load(string path) {
    ;
}
