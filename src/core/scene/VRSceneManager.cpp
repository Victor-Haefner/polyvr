#include "VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"
#include "core/utils/VRRate.h"
#include "VRScene.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include <OpenSG/OSGSceneFileHandler.h>
#include <gtkmm/main.h>
#include <GL/glut.h>


OSG_BEGIN_NAMESPACE
using namespace std;

void glutUpdate() {
    //osgsleep(1);
    VRSceneManager::get()->update();
}

int gtkUpdate(void* data) {
    VRSceneManager::get()->update();
    return true;
}

VRSceneManager::VRSceneManager() {
    cout << "Init VRSceneManager\n";
    active = "NO_SCENE_ACTIVE";

    //g_timeout_add(17, gtkUpdate, NULL); // 60 Hz
    g_timeout_add_full(G_PRIORITY_LOW, 17, gtkUpdate, NULL, NULL); // 60 Hz
    glutDisplayFunc(glutUpdate);
    glutIdleFunc(glutUpdate);
}

VRSceneManager::~VRSceneManager() { for (auto scene : scenes) delete scene.second; }

void VRSceneManager::operator= (VRSceneManager v) {;}

VRSceneManager* VRSceneManager::get() {
    static VRSceneManager* mgr = new VRSceneManager();
    return mgr;
}

void VRSceneManager::addScene(VRScene* s) {
    scenes[s->getName()] = s;
    setActiveScene(s);
}

void VRSceneManager::removeScene(VRScene* s) {
    if (s == 0) return;
    scenes.erase(s->getName());
    delete s;

    VRSetupManager::get()->getCurrent()->resetViewports();
    VRSetupManager::get()->getCurrent()->clearSignals();
    VRTransform::changedObjects.clear();
    VRTransform::dynamicObjects.clear();
    active = "NO_SCENE_ACTIVE";

    // deactivate windows
    map<string, VRWindow*> windows = VRSetupManager::get()->getCurrent()->getWindows();
    map<string, VRWindow*>::iterator itr;
    for (itr = windows.begin(); itr != windows.end(); itr++) itr->second->setContent(false);
}

void VRSceneManager::newScene(string path) {
    removeScene(getActiveScene());

    VRScene* scene = new VRScene();
    scene->setPath(path);
    scene->setName(scene->getFileName());
    VRTransform* cam = scene->addCamera("Default");

    VRLight* headlight = scene->addLight("Headlight");
    headlight->setType("point");
    VRLightBeacon* headlight_B = new VRLightBeacon("Headlight_beacon");
    headlight->setBeacon(headlight_B);
    VRTransform* user = VRSetupManager::get()->getCurrent()->getUser();
    scene->add(headlight);
    headlight->addChild(cam);
    if (user) user->addChild(headlight_B);
    else cam->addChild(headlight_B);

    cam->setFrom(Vec3f(0,0,3));
    addScene(scene);
}

void VRSceneManager::setActiveScene(VRScene* s) {
    if (scenes.size() == 0) { cout << "\n ERROR: No scenes defined " << flush; return; }

    if (scenes.count(s->getName()) == 0) { cout << "\n ERROR: No scene " << s->getName() << flush; return; }
    else s->getRoot()->show(); //activate new scene

    if (active != "NO_SCENE_ACTIVE") scenes[active]->getRoot()->hide(); //hide old scene

    active = s->getName();
    VRSetupManager::get()->getCurrent()->setScene(s);

    // todo:
    //  - add scene signals to setup devices
    //  - the setup mouse needs the active camera
    //  - setup views need the scene root for rendering
    //  - the setup real_root has to be added to the current camera
}

void VRSceneManager::setActiveSceneByName(string s) { if (scenes.count(s) == 1) setActiveScene(scenes[s]); }

//void printTree() { scenes[active]->printTree();}

int VRSceneManager::getSceneNum() {return scenes.size();}

VRScene* VRSceneManager::getScene(string s) { if (scenes.count(s)) return scenes[s]; else return 0; }

VRScene* VRSceneManager::getActiveScene() { return getScene(active); }

void VRSceneManager::update() {
    int fps = VRRate::get()->getRate();

    if (scenes.count(active) == 1) {
        //if (active == "NO_SCENE_ACTIVE") cout << "\nupdate " << active << flush; //TODO: segfault when closing scene
        if (scenes[active] != 0) {
            scenes[active]->update();
            VRSetupManager::get()->getCurrent()->updateActivatedSignals();
        }
    }

    updateCallbacks();

    VRSetupManager::get()->getCurrent()->updateWindows();//rendering

    VRGlobals::get()->CURRENT_FRAME++;
    VRGlobals::get()->FRAME_RATE = fps;
}

OSG_END_NAMESPACE
