#include "VRSceneManager.h"
#include "VRSceneLoader.h"
#include "VRScene.h"
#include "VRProjectsList.h"

#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRRate.h"
#include "core/utils/VRProfiler.h"
#include "core/utils/coreDumpHandler.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRFunction.h"
#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiSignals.h"
#include "core/gui/VRGuiFile.h"
#include "core/gui/VRGuiUtils.h"
#endif

#include <OpenSG/OSGSceneFileHandler.h>
#include <boost/filesystem.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <time.h>
#include <thread>

typedef boost::recursive_mutex::scoped_lock PLock;

OSG_BEGIN_NAMESPACE

VRSceneManager* main_instance = 0;

VRSceneManager::VRSceneManager() {
    cout << "Init VRSceneManager..";
    main_instance = this;
	original_workdir = boost::filesystem::current_path().string();
	examples = VRProjectsList::create();
	projects = VRProjectsList::create();
    searchExercisesAndFavorites();

    on_scene_load = VRSignal::create();
    on_scene_close = VRSignal::create();

    VROntology::setupLibrary();
    cout << " done" << endl;

    sceneUpdateCb = VRThreadCb::create( "update scene", bind(&VRSceneManager::updateSceneThread, this, _1) );
    //initThread(sceneUpdateCb, "update scene", true, 1); // TODO
}

VRSceneManager::~VRSceneManager() {
    cout << "VRSceneManager::~VRSceneManager" << endl;
    main_instance = 0;
}

VRSceneManagerPtr VRSceneManager::create() { return VRSceneManagerPtr( new VRSceneManager()); }

VRSceneManager* VRSceneManager::get() {
    if (main_instance == 0) cout << " -- Warning! VRSceneManager::get called during shutdown!" << endl;
    return main_instance;
}

void VRSceneManager::loadScene(string path, bool write_protected, string encryptionKey) {
    if (!exists(path)) { cout << "VRSceneManager, loadScene: " << path << " not found" << endl; return; }
    path = canonical(path);
    if (current) if (current->getPath() == path) return;
    cout << "VRSceneManager, loadScene: " << path << endl;

    newEmptyScene(path);
    bool success = VRSceneLoader::get()->loadScene(path, encryptionKey);
    if (!success) {
#ifndef WITHOUT_GTK
        notifyUser("Could not load scene", "File '" + path + "' not found or corrupted!");
#endif
        return;
    }
    current->setFlag("write_protected", write_protected);

#ifndef WITHOUT_GTK
    VRGuiSignals::get()->getSignal("scene_changed")->triggerPtr<VRDevice>(); // update gui
#endif

    cout << " VRSceneManager, storeFavorites" << endl;
    if (auto pEntry = projects->getEntry(path)) {
        pEntry->setTimestamp(toString(time(0)));
        storeFavorites();
    }
    cout << " VRSceneManager, loadScene done" << endl;
}

string VRSceneManager::getOriginalWorkdir() { return original_workdir; }

void VRSceneManager::closeScene() {
    if (current == 0) return;
#ifndef WASM
    VRProfiler::get()->setActive(false);
#endif
    on_scene_close->triggerPtr<VRDevice>();
    current = 0;

    auto setup = VRSetup::getCurrent();
    setup->resetViewports();
    setup->clearSignals();
    VRTransform::dynamicObjects.clear();

    for (auto w : setup->getWindows()) {
        cout << "VRSceneManager::closeScene!" << endl;
        w.second->setContent(false); // deactivate windows
        w.second->clear(Color3f(0.2,0.2,0.2)); // render last time
    }
    setWorkdir(original_workdir);

#ifndef WITHOUT_GTK
    VRGuiSignals::get()->getSignal("scene_changed")->triggerPtr<VRDevice>(); // update gui
#endif
}

void VRSceneManager::reloadScene() {
    if (current == 0) return;
    string path = current->getPath();
    closeScene();
    loadScene(path);
}

void VRSceneManager::setWorkdir(string path) {
	if (path == "") return;
	if (exists(path)) path = canonical(path);
	boost::filesystem::current_path(path);
	clearDumpFiles();
}

void VRSceneManager::newEmptyScene(string path) {
    cout << "VRSceneManager::newEmptyScene: " << path << endl;
    closeScene();
    VRScenePtr scene = VRScenePtr( new VRScene() );
    scene->setPath(path);
    setWorkdir(scene->getWorkdir());
    scene->setName(scene->getFileName());
    current = scene;
    cout << " VRSceneManager::newEmptyScene done" << endl;
}

void VRSceneManager::newScene(string path) {
    cout << "VRSceneManager::newScene at " << path << endl;
    if (exists(path)) path = canonical(path);
    newEmptyScene(path);

    VRLightPtr headlight = VRLight::create("light");
    headlight->setType("point");
    VRLightBeaconPtr headlight_B = VRLightBeacon::create("Headlight_beacon");
    headlight->setBeacon(headlight_B);
    current->add(headlight);

    VRCameraPtr cam = VRCamera::create("Default");
    cam->activate();
    headlight->addChild(cam);

    VRTransformPtr user;
    auto setup = VRSetup::getCurrent();
    if (setup) user = setup->getUser();
    if (user) user->addChild(headlight_B);
    else cam->addChild(headlight_B);

    cam->setFrom(Vec3d(3,2,3));
    cam->setAt(Vec3d(0,0,0));

    string initScript = "\timport VR\n\n\tif hasattr(VR, 'scene'): VR.scene.destroy()\n\tVR.scene = VR.Object('scene', 'light')\n\n\t";
    current->newScript("init", initScript);
    setScene(current);
}

VRSignalPtr VRSceneManager::getSignal_on_scene_load() { return on_scene_load; }
VRSignalPtr VRSceneManager::getSignal_on_scene_close() { return on_scene_close; }

void VRSceneManager::setScene(VRScenePtr scene) {
	cout << "VRSceneManager::setScene " << scene << endl;
    if (!scene) return;
    current = scene;
    auto setup = VRSetup::getCurrent();
    if (setup) setup->setScene(scene);
    scene->setActiveCamera();
#ifndef WASM
    VRProfiler::get()->setActive(true);
#endif

    on_scene_load->triggerPtr<VRDevice>();

#ifndef WITHOUT_GTK
    VRGuiSignals::get()->getSignal("scene_changed")->triggerPtr<VRDevice>(); // update gui
#endif
	cout << " VRSceneManager::setScene done" << endl;
}

void VRSceneManager::storeFavorites() {
    string path = original_workdir + "/examples/.config";
    projects->saveToFile(path);
}

void VRSceneManager::addFavorite(string path) {
    if (projects->hasEntry(path)) return;
    projects->addEntry( VRProjectEntry::create(path, ""));
    storeFavorites();
}

void VRSceneManager::remFavorite(string path) {
    projects->remEntry(path);
    storeFavorites();
}

void VRSceneManager::searchExercisesAndFavorites() {
#ifndef WITHOUT_GTK
    projects->clear();
    examples->clear();

    // examples
	vector<string> files = VRGuiFile::listDir("examples");
	for (string file : files) {
		int N = file.size(); if (N < 6) continue;

		string ending = file.substr(N - 4, N - 1);
		if (ending != ".xml" && ending != ".pvr") continue;

		string path = canonical("examples/" + file);
		examples->addEntry( VRProjectEntry::create(path, "") );
	}

    // favorites
    ifstream file( "examples/.cfg" ); // check for old config file
    if (file.is_open()) {
        string line, path;
        while ( getline (file,line) ) {
            ifstream f(line.c_str());
            if (!f.good()) continue;
            f.close();
            line = canonical(line);
            projects->addEntry( VRProjectEntry::create(line, "") );
        }
        file.close();
        storeFavorites();
        remove("examples/.cfg"); // remove old config file
        return;
    }

    projects->loadFromFile("examples/.config");
#endif
}

VRProjectsListPtr VRSceneManager::getFavoritePaths() { return projects; }
VRProjectsListPtr VRSceneManager::getExamplePaths() { return examples; }

VRScenePtr VRSceneManager::getCurrent() { return current; }

void VRSceneManager::updateSceneThread(VRThreadWeakPtr tw) {
    updateScene();
	std::this_thread::sleep_for(chrono::milliseconds(1));
}

void VRSceneManager::updateScene() {
    if (!current) return;
    if (auto setup = VRSetup::getCurrent()) setup->updateActivatedSignals();
    current->update();
}

void VRSceneManager::setTargetFPS(double fps) { targetFPS = fps;  }

void VRSceneManager::update() {
    VRProfiler* profiler = 0;
    VRTimer timer;
    int fps = 0, pID1 = 0;

    auto setupProfiling = [&]() {
#ifndef WASM
        profiler = VRProfiler::get();
        profiler->swap();
        pID1 = profiler->regStart("frame");
#endif
        static VRRate FPS;
        fps = FPS.getRate();
        timer.start();
    };

    auto doGTKUpdate = [&]() {
#ifndef WASM
        int pID7 = profiler->regStart("frame gtk update");
#endif
        VRTimer t1; t1.start();
#ifndef WITHOUT_GTK
        VRGuiManager::get()->updateGtk(); // update GUI
#endif
        VRGlobals::GTK1_FRAME_RATE.update(t1);
        VRGlobals::UPDATE_LOOP1.update(timer);
#ifndef WASM
        profiler->regStop(pID7);
#endif
    };

    auto doCallbacks = [&]() {
#ifndef WASM
        int pID6 = profiler->regStart("frame callbacks");
#endif
        VRTimer t4; t4.start();
        updateCallbacks();
        VRGlobals::SMCALLBACKS_FRAME_RATE.update(t4);
        VRGlobals::UPDATE_LOOP2.update(timer);
#ifndef WASM
        profiler->regStop(pID6);
#endif
    };

    auto doSetupUpdate = [&]() {
#ifndef WASM
        int pID5 = profiler->regStart("frame devices");
#endif
        VRTimer t5; t5.start();
        if (auto setup = VRSetup::getCurrent()) {
            setup->updateTracking(); // tracking
            setup->updateDevices(); // device beacon update
        }
#ifndef WASM
        profiler->regStop(pID5);
#endif
        VRGlobals::SMCALLBACKS_FRAME_RATE.update(t5);
        VRGlobals::UPDATE_LOOP3.update(timer);
    };

    auto doSceneUpdate = [&]() {
#ifndef WASM
        int pID4 = profiler->regStart("frame scene");
#endif
        VRTimer t6; t6.start();
        updateScene();
        VRGlobals::SCRIPTS_FRAME_RATE.update(t6);
        VRGlobals::UPDATE_LOOP4.update(timer);
#ifndef WASM
        profiler->regStop(pID4);
#endif
    };

    auto doRender = [&]() {
#ifndef WASM
        int pID3 = profiler->regStart("frame draw");
#endif
        if (auto setup = VRSetup::getCurrent()) {
            VRTimer t2; t2.start();
            setup->updateWindows(); // rendering
            setup.reset(); // updateGtk may close application, reset setup to avoid memory leak
            VRGlobals::WINDOWS_FRAME_RATE.update(t2);
            VRGlobals::UPDATE_LOOP5.update(timer);
#ifndef WASM
            int pID31 = profiler->regStart("frame gtk update");
#endif
#ifndef WITHOUT_GTK
            VRGuiManager::get()->updateGtk();
#endif
#ifndef WASM
            profiler->regStop(pID31);
#endif
            VRGlobals::UPDATE_LOOP6.update(timer);
        }
#ifndef WASM
        profiler->regStop(pID3);
#endif
    };

    auto doSleep = [&]() {
        if (current) current->allowScriptThreads();
        VRGlobals::CURRENT_FRAME++;
        VRGlobals::FRAME_RATE.fps = fps;
        VRTimer t7; t7.start();
#ifndef WASM // main loop is controlled by wasm, no sleep needed here
        int pID2 = profiler->regStart("frame sleep");
        doFrameSleep(timer.stop(), targetFPS);
        profiler->regStop(pID2);
#endif
        VRGlobals::SLEEP_FRAME_RATE.update(t7);
        if (current) current->blockScriptThreads();
#ifndef WASM
        profiler->regStop(pID1);
#endif
        //cout << " VRSceneManager::update done" << endl;

#ifdef WASM
        VRSetup::getCurrent()->getView(0)->updateStatsEngine();
#endif
        VRGlobals::UPDATE_LOOP7.update(timer);
    };

    setupProfiling();
    doGTKUpdate();
    doCallbacks();
    doSetupUpdate();
    doSceneUpdate();
    doRender();
    doSleep();
}

OSG_END_NAMESPACE
