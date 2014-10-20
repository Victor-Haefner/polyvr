#include "VRViewManager.h"
#include "VRView.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

bool VRViewManager::checkView(int i) {
    if (i == -1) return false;
    else if (views.count(i)) return true;
    else {
        cout << "\nError! request for not existing view " << i << "\n";
        return false;
    }
}

void VRViewManager::setViewAnchor(VRTransform* a) { anchor = a; }

void VRViewManager::showViewStats(int i, bool b) {
    if (checkView(i)) views[i]->showStats(b);
}

VRViewManager::VRViewManager() {
    cout << "Init VRViewManager\n";
    //this->view_root = view_root;
}

VRViewManager::~VRViewManager() {
    for (itr = views.begin(); itr != views.end(); itr++) delete itr->second;
}

//int VRViewManager::addView(bool active_stereo, bool stereo, bool projection, Pnt3f screenLowerLeft, Pnt3f screenLowerRight, Pnt3f screenUpperRight, Pnt3f screenUpperLeft, bool swapeyes) {
int VRViewManager::addView(string name) {
    //VRView* view = new VRView(active_stereo, stereo, projection, screenLowerLeft, screenLowerRight, screenUpperRight, screenUpperLeft, swapeyes);
    VRView* view = new VRView(name);
    int id=0;
    while(views.count(id) == 1) id++;
    views[id] = view;
    view->setRoot(0, anchor);
    view->setID(id);
    return id;
}

void VRViewManager::setViewCamera(VRCamera* c, int i) {
    if (checkView(i)) views[i]->setCamera(c);
    else if(i == -1) for (itr = views.begin(); itr != views.end(); itr++) itr->second->setCamera(c);
}

void VRViewManager::setViewRoot(VRObject* root, int i) {
    if (checkView(i)) views[i]->setRoot(root, anchor);
    else if(i == -1) for (itr = views.begin(); itr != views.end(); itr++) itr->second->setRoot(root, anchor);
}

void VRViewManager::setViewUser(VRTransform* user, int i) { // not used
    anchor->addChild(user);
    if (checkView(i)) views[i]->setUser(user);
    else if(i == -1) for (itr = views.begin(); itr != views.end(); itr++) itr->second->setUser(user);
}

void VRViewManager::setViewBackground(BackgroundRecPtr bg, int i) {
    if (checkView(i)) views[i]->setBackground(bg);
    else if(i == -1) for (itr = views.begin(); itr != views.end(); itr++) itr->second->setBackground(bg);
}

void VRViewManager::showViewportGeos(bool b) {
    for (itr = views.begin(); itr != views.end(); itr++) itr->second->showViewGeo(b);
}

void VRViewManager::removeView(int i) {
    if (!checkView(i)) return;
    delete views[i];
    views.erase(i);
}

//void VRViewManager::addRWElement(VRTransform* obj) { view_root->addChild(obj); }

VRTransform* VRViewManager::getViewUser(int i) { if (checkView(i)) return views[i]->getUser(); else return 0; }

VRView* VRViewManager::getView(int i) { if (checkView(i)) return views[i]; else return 0; }

void VRViewManager::setStereo(bool b) { for (itr = views.begin(); itr != views.end(); itr++) itr->second->setStereo(b); }
void VRViewManager::setStereoEyeSeparation(float v) { for (itr = views.begin(); itr != views.end(); itr++) itr->second->setStereoEyeSeparation(v); }

void VRViewManager::resetViewports() {
    for (itr = views.begin(); itr != views.end(); itr++) itr->second->reset();
}

void VRViewManager::setFotoMode(bool b) {
    for (itr = views.begin(); itr != views.end(); itr++) itr->second->setFotoMode(b);
}

void VRViewManager::setCallibrationMode(bool b) {
    for (itr = views.begin(); itr != views.end(); itr++) itr->second->setCallibrationMode(b);
}

OSG_END_NAMESPACE;
