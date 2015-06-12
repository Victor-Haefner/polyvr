#include "VRSceneManager.h"
#include "VRSceneLoader.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"
#include "core/utils/VRRate.h"
#include "core/utils/VRProfiler.h"
#include "VRScene.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/VRTimer.h"
#include "core/gui/VRGuiSignals.h"
#include "core/gui/VRGuiFile.h"
#include <OpenSG/OSGSceneFileHandler.h>
#include <gtkmm/main.h>
#include <GL/glut.h>
#include <boost/filesystem.hpp>

OSG_BEGIN_NAMESPACE
using namespace std;

VRSceneManager::VRSceneManager() {
    cout << "Init VRSceneManager\n";
    active = "NO_SCENE_ACTIVE";
	original_workdir = boost::filesystem::current_path().string();
	cout << " PolyVR system directory: " << original_workdir << endl;
    searchExercisesAndFavorites();

    on_scene_load = new VRSignal();
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
    VRGuiSignals::get()->getSignal("scene_changed")->trigger<VRDevice>(); // update gui
}

void VRSceneManager::loadScene(string path, bool write_protected) {
    if (!boost::filesystem::exists(path)) { cout << "loadScene " << path << " not found" << endl; return; }
    path = boost::filesystem::canonical(path).string();
    cout << "loadScene " << path << endl;
    if (getCurrent()) if (getCurrent()->getPath() == path) return;

    removeScene(getCurrent());
    VRSceneLoader::get()->loadScene(path);
    VRSceneManager::getCurrent()->setFlag("write_protected", write_protected);
    VRGuiSignals::get()->getSignal("scene_changed")->trigger<VRDevice>(); // update gui
}

string VRSceneManager::getOriginalWorkdir() { return original_workdir; }

void VRSceneManager::removeScene(VRScene* s) {
    if (s == 0) return;
    scenes.erase(s->getName());
    active = "NO_SCENE_ACTIVE";
    delete s;

    VRSetupManager::getCurrent()->resetViewports();
    VRSetupManager::getCurrent()->clearSignals();
    VRTransform::dynamicObjects.clear();

    // deactivate windows
    auto windows = VRSetupManager::getCurrent()->getWindows();
    for (auto w : windows) w.second->setContent(false);

    setWorkdir(original_workdir);
    VRGuiSignals::get()->getSignal("scene_changed")->trigger<VRDevice>(); // update gui
}

void VRSceneManager::setWorkdir(string path) {
    if (path == "") return;
    string full_path = path[0] != '/' ? original_workdir + '/' + path : path;
	boost::filesystem::current_path(full_path);
}

void VRSceneManager::newScene(string path) {
    path = boost::filesystem::canonical(path).string();
    removeScene(getCurrent());

    VRScene* scene = new VRScene();
    scene->setPath(path);
    setWorkdir(scene->getWorkdir());
    scene->setName(scene->getFileName());
    VRTransform* cam = scene->addCamera("Default");

    VRLight* headlight = scene->addLight("Headlight");
    headlight->setType("point");
    VRLightBeacon* headlight_B = new VRLightBeacon("Headlight_beacon");
    headlight->setBeacon(headlight_B);
    VRTransform* user = VRSetupManager::getCurrent()->getUser();
    scene->add(headlight);
    headlight->addChild(cam);
    if (user) user->addChild(headlight_B);
    else cam->addChild(headlight_B);

    cam->setFrom(Vec3f(0,0,3));
    addScene(scene);
}

VRSignal* VRSceneManager::getSignal_on_scene_load() { return on_scene_load; }

void VRSceneManager::setActiveScene(VRScene* s) {
    if (scenes.size() == 0) { cout << "\n ERROR: No scenes defined " << flush; return; }

    if (scenes.count(s->getName()) == 0) { cout << "\n ERROR: No scene " << s->getName() << flush; return; }
    else s->getRoot()->show(); //activate new scene

    if (active != "NO_SCENE_ACTIVE") scenes[active]->getRoot()->hide(); //hide old scene

    active = s->getName();
    VRSetupManager::getCurrent()->setScene(s);
    s->setActiveCamera(0);

    on_scene_load->trigger<VRDevice>();

    // todo:
    //  - add scene signals to setup devices
    //  - the setup mouse needs the active camera
    //  - setup views need the scene root for rendering
    //  - the setup real_root has to be added to the current camera
}

void VRSceneManager::storeFavorites() {
    string path = original_workdir + "/examples/.cfg";
    ofstream file(path);
    for (auto f : favorite_paths) file << f << endl;
    file.close();
}

void VRSceneManager::addFavorite(string path) {
    for (auto p : favorite_paths) if (p == path) return;
    favorite_paths.push_back(path);
    storeFavorites();
}

void VRSceneManager::remFavorite(string path) {
    favorite_paths.erase(std::remove(favorite_paths.begin(), favorite_paths.end(), path), favorite_paths.end());
    storeFavorites();
}

void VRSceneManager::searchExercisesAndFavorites() {
    favorite_paths.clear();
    example_paths.clear();

    // examples
	vector<string> files = VRGuiFile::listDir("examples");
	for (string file : files) {
		int N = file.size(); if (N < 6) continue;

		string ending = file.substr(N - 4, N - 1);
		if (ending != ".xml") continue;

		string path = boost::filesystem::canonical("examples/" + file).string();
		example_paths.push_back(path);
	}

    // favorites
    ifstream file( "examples/.cfg" );
    if (!file.is_open()) return;

    string line, path;
    while ( getline (file,line) ) {
        ifstream f(line.c_str());
        if (!f.good()) continue;
		line = boost::filesystem::canonical(line).string();
        favorite_paths.push_back(line);
    }
    file.close();
}

vector<string> VRSceneManager::getFavoritePaths() { return favorite_paths; }
vector<string> VRSceneManager::getExamplePaths() { return example_paths; }

void VRSceneManager::setActiveSceneByName(string s) { if (scenes.count(s) == 1) setActiveScene(scenes[s]); }

int VRSceneManager::getSceneNum() {return scenes.size();}

VRScene* VRSceneManager::getScene(string s) { if (scenes.count(s)) return scenes[s]; else return 0; }

VRScene* VRSceneManager::getCurrent() { return get()->getScene(get()->active); }

void sleep_to(int fps) {
    int dt = VRTimer::getBeacon("st");
    if (dt > 16) return;
    osgSleep(16-dt);
}

void VRSceneManager::updateScene() {
    if (scenes.count(active) == 1) {
        if (scenes[active] != 0) {
            VRSetupManager::getCurrent()->updateActivatedSignals();
            scenes[active]->update();
        }
    }
}

void VRSceneManager::update() {
    VRProfiler::get()->swap();
    int fps = VRRate::get()->getRate();

    VRGuiManager::get()->updateGtk();
    updateCallbacks();
    VRSetupManager::getCurrent()->updateDevices();//device beacon update
    updateScene();

    if (VRSetupManager::getCurrent()) VRSetupManager::getCurrent()->updateWindows();//rendering
    VRGuiManager::get()->updateGtk();

    VRGlobals::get()->CURRENT_FRAME++;
    VRGlobals::get()->FRAME_RATE = fps;

    sleep_to(60);
}

OSG_END_NAMESPACE
