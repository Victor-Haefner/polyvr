#include "VRScene.h"
#include "core/scripting/VRScript.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#endif
#include "core/setup/devices/VRFlystick.h"
#include "core/setup/devices/VRMouse.h"
#ifndef WITHOUT_MTOUCH
#include "core/setup/devices/VRMultiTouch.h"
#endif
#include "core/setup/devices/VRKeyboard.h"
#ifndef WITHOUT_BULLET
#include "core/setup/devices/VRHaptic.h"
#endif
#include "core/setup/devices/VRServer.h"
#include "VRSceneManager.h"
#include "VRSemanticManager.h"
#include "core/setup/VRSetup.h"
#include "core/objects/OSGObject.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRGroup.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRTimer.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/utils/system/VRSystem.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScene::VRScene() {
    cout << "Init Scene" << endl;

    setNameSpace("Scene");
    setName("Scene");

    icon = "Scene.png";
    setFlag("write_protected", false);

    //scene update functions
    addUpdateFkt(updateAnimationsFkt);
#ifndef WITHOUT_BULLET
    addUpdateFkt(updatePhysObjectsFkt);
    physicsThreadID = initThread(updatePhysicsFkt, "physics", true, 0);
#endif

    loadingTimeCb = VRUpdateCb::create("loadingTimeCb", bind(&VRScene::recLoadingTime, this));

    initDevices();
    VRMaterial::getDefault()->resetDefault();

    referentials_layer = VRVisualLayer::getLayer("Referentials", "refs.png", 1);
    cameras_layer = VRVisualLayer::getLayer("Cameras", "cameras.png", 1);
    lights_layer = VRVisualLayer::getLayer("Lights", "lights.png", 1);

    layer_ref_toggle = VRFunction<bool>::create("showReferentials", bind(&VRScene::showReferentials, this, _1, (VRObjectPtr)0) );
    layer_cam_toggle = VRFunction<bool>::create("showCameras", bind(&VRScene::showCameras, this, _1) );
    layer_light_toggle = VRFunction<bool>::create("showLights", bind(&VRScene::showLights, this, _1) );
    referentials_layer->setCallback( layer_ref_toggle );
    cameras_layer->setCallback( layer_cam_toggle );
    lights_layer->setCallback( layer_light_toggle );

    VRVisualLayer::anchorLayers(root);

    semanticManager = VRSemanticManager::create();
    loadingProgress = VRProgress::create("Loading Scene", 100, VRProgress::CALLBACK_M);

    cout << " init scene done\n";
}

VRScene::~VRScene() {
    //kill physics thread
    cout << "VRScene::~VRScene " << name << endl;
    VRThreadManager::stopThread(physicsThreadID);
    root->destroy();
    VRGroup::clearGroups();
    VRLightBeacon::getAll().clear();
    auto setupCam = *VRCamera::getAll().begin();
    VRCamera::getAll().clear();
    VRCamera::getAll().push_back(setupCam);
    VRMaterial::clearAll();
}

VRScenePtr VRScene::getCurrent() { return VRSceneManager::get()->getCurrent(); }

void VRScene::initDevices() { // TODO: remove this after refactoring the navigation stuff
    auto setup = VRSetup::getCurrent();
    if (!setup) return;

    VRMousePtr mouse = dynamic_pointer_cast<VRMouse>( setup->getDevice("mouse") );
    VRFlystickPtr flystick = dynamic_pointer_cast<VRFlystick>( setup->getDevice("flystick") );
    VRDevicePtr razer = setup->getDevice("vrpn_device");

    if (mouse) {
        initOrbit(getActiveCamera(), mouse); //TODO, load from xml
        //initWalk(getActiveCamera(), mouse);
        //initOrbit2D(getActiveCamera(), mouse);
        setActiveNavigation("Orbit");
    }

    if (flystick) {
        initFlyWalk(getActiveCamera(), flystick); // TODO
        setActiveNavigation("FlyWalk");
    }

    if (razer) {
        initHydraFly(getActiveCamera(), razer); // TODO
        setActiveNavigation("Hydra");
    }

    setup->resetDeviceDynNodes(getRoot());

    cout << " init devices done\n";
}

void VRScene::setPath(string path) { this->path = path; }
string VRScene::getPath() { return path; }
string VRScene::getFileName() {
    return getFile().substr(0,getFile().size()-4);
}
string VRScene::getIcon() { return ".local_" + getFileName() + "/snapshot.png"; }
string VRScene::getFile() {
    int n = path.find_last_of("\\/");
    if (n == -1) return path;
    return path.substr(path.find_last_of("\\/")+1, path.size()-1);
}

string VRScene::getWorkdir() {
    int n = path.find_last_of("\\/");
    if (n == -1) return ".";
    string wdir = path.substr(0, n);
    //cout << "getWorkdir from path " << path << " which is " << wdir << endl;
    return wdir;
}

void VRScene::add(VRObjectPtr obj, int parentID) {
    if (obj == 0) return;

    VRObjectPtr o = get(parentID);
    if (o != 0) o->addChild(obj);
    else root->addChild(obj);
}

void VRScene::add(NodeMTRecPtr n) { root->addChild(OSGObject::create(n)); }

VRObjectPtr VRScene::get(int ID) {
    if (ID == -1) return 0;
    VRObjectPtr o = 0;
    o = root->find(ID);
    return o;
}

VRObjectPtr VRScene::get(string name, bool strict) {
    if (strict) return root->find(name);
    else        return root->findFirst(name);
}

void VRScene::setActiveCamera(string camname) {
    cout << "set active camera: " << camname << endl;
    //printBacktrace();
    setMActiveCamera(camname);
    auto setup = VRSetup::getCurrent();
    if (!setup) return;

    // TODO: refactor the following workaround
    VRCameraPtr cam = getActiveCamera();
    if (cam == 0) return;

    VRMousePtr mouse = dynamic_pointer_cast<VRMouse>( setup->getDevice("mouse") );
    VRFlystickPtr flystick = dynamic_pointer_cast<VRFlystick>( setup->getDevice("flystick") );
    VRDevicePtr razer = setup->getDevice("vrpn_device");
    VRServerPtr server1 = dynamic_pointer_cast<VRServer>( setup->getDevice("server1") );

    if (mouse) {
        mouse->setTarget(cam);
        mouse->setCamera(cam);
    }

#ifndef WITHOUT_MTOUCH
    VRMultiTouchPtr multitouch = dynamic_pointer_cast<VRMultiTouch>( setup->getDevice("multitouch") );
    if (multitouch) {
        multitouch->setTarget(cam);
        multitouch->setCamera(cam);
    }
#endif

    if (flystick) flystick->setTarget(cam);
    if (razer) razer->setTarget(cam);
    if (server1) server1->setTarget(cam);

    setup->setViewCamera(cam, -1);
    if (cam->getAcceptRoot()) {
        setup->getRoot()->switchParent(cam);
        //setup->getRoot()->switchParent(cam->getSetupNode());
    }
}

VRObjectPtr VRScene::getRoot() { return root; }
//VRObjectPtr VRScene::getSystemRoot() { return root_system; }

void VRScene::printTree() { root->printTree(); }

void VRScene::showReferentials(bool b, VRObjectPtr o) {
    if (o == 0) o = root;

    VRTransformPtr t = 0;
    if (o->hasTag("transform")) t = static_pointer_cast<VRTransform>(o);
    if (t) t->showCoordAxis(b);

    for (unsigned int i=0; i<o->getChildrenCount(); i++) showReferentials(b, o->getChild(i));
}

void VRScene::showLights(bool b) { for (auto be : VRLightBeacon::getAll()) be.lock()->showLightGeo(b); }
void VRScene::showCameras(bool b) { for (auto c : VRCamera::getAll()) if (auto sp = c.lock()) sp->showCamGeo(b); }

void VRScene::update() {
    //cout << " VRScene::update" << endl;
    //Vec3d min,max;
    //root->getNode()->updateVolume();
    //root->getNode()->getVolume().getBounds( min, max );
    ThreadManagerUpdate();
    updateCallbacks();
    VRTransform::updateConstraints();
    //cout << "  VRScene::update done" << endl;
}

void VRScene::updateLoadingProgress(VRThreadWeakPtr t) {
    if (loadingTime <= 0) return;
    float last = loadingProgress->get();
    float current = float(loadingTimer.stop())/loadingTime;
    int delta = floor(100*(current-last));
    loadingProgress->update(delta);
    osgSleep(10); // slow down progress feedback!
}

void VRScene::recLoadingTime() {
    cout << "expected loading time: " << loadingTime*0.001 << " s" << endl;
    VRSceneManager::get()->stopThread(loadingProgressThread);
    loadingProgress->finish();
    loadingTime = loadingTimer.stop();
    cout << "measured loading time: " << loadingTime*0.001 << " s" << endl;
}

VRProgressPtr VRScene::getLoadingProgress() { return loadingProgress; }

void mkPath(string path) {
    auto dirs = splitString(path, '/');
    path = "";
    for (unsigned int i=1; i<dirs.size(); i++) {
        path += "/"+dirs[i];
        makedir(path);
    }
}

void VRScene::saveScene(XMLElementPtr e) {
    if (e == 0) return;
    VRName::save(e);
    VRCameraManager::saveUnder(e);
    VRRenderManager::saveUnder(e);
    VRScriptManager::saveUnder(e);
    VRNetworkManager::saveUnder(e);
    VRBackground::saveUnder(e);
    VRNavigator::saveUnder(e);
    VRMaterialManager::saveUnder(e);
    semanticManager->saveUnder(e);

    string d = getWorkdir() + "/.local_"+getFileName();
    if (!exists(d)) mkPath(d);
    ofstream stats(d+"/stats");
    string lt = toString(loadingTime);
    stats.write( lt.c_str(), lt.size() );
    stats.close();
}

void VRScene::loadScene(XMLElementPtr e) {
    cout << "VRScene::loadScene" << endl;
    if (e == 0) return;

    string d = getWorkdir() + "/.local_"+getFileName()+"/stats";
    if (exists(d)) {
        ifstream stats(d);
        stats >> loadingTime;
        stats.close();
    }

    loadingTimer.start();
    loadingProgressThreadCb = VRFunction< VRThreadWeakPtr >::create( "loading progress thread", bind(&VRScene::updateLoadingProgress, this, _1) );
    loadingProgressThread = VRSceneManager::get()->initThread(loadingProgressThreadCb, "loading progress thread", true, 1);

    VRName::load(e);
    VRCameraManager::loadChildFrom(e);
    VRRenderManager::loadChildFrom(e);
    VRScriptManager::loadChildFrom(e);
    VRScriptManager::triggerOnLoad();
    VRNetworkManager::loadChildFrom(e);
    VRBackground::loadChildFrom(e);
    VRNavigator::loadChildFrom(e);
    VRMaterialManager::loadChildFrom(e);
    semanticManager->loadChildFrom(e);

    VRCameraManager::CMsetup();
    VRRenderManager::update();
    VRScriptManager::update();
    VRNetworkManager::update();
    VRBackground::update();
    VRNavigator::update();
    VRMaterialManager::update();
    semanticManager->update();

    queueJob(loadingTimeCb, 0, 2);
    cout << " VRScene::loadScene done" << endl;
}

/**

TODO:

- when calling VR.importScene(..)
    - execute on_import triggered scripts
- when calling VR.importScene(..) repeatedly
    - destroy old imported scene
    - reload scripts
- on_import trigger
    - loading scripts does not override if key present..
    - execute each time importScene is called!
*/

void VRScene::importScene(XMLElementPtr e, string path) {
    if (e == 0) return;
    auto oldScripts = getScripts();
    VRScriptManager::loadChildFrom(e);

    vector<string> newScripts;
    for (auto s : getScripts()) if (!oldScripts.count(s.first)) newScripts.push_back(s.first);
    if (!importedScripts.count(path)) {
        importedScripts[path] = vector<string>();
        for (auto s : newScripts) importedScripts[path].push_back(s);
    } else newScripts = importedScripts[path];

    for (auto s : newScripts) getScript(s)->setPersistency(0);
    for (auto s : newScripts) updateScript(s, getScript(s)->getCore());
    for (auto s : newScripts) if (getScript(s)->hasTrigger("on_scene_import")) getScript(s)->execute();
#ifndef WITHOUT_GTK
    VRGuiManager::get()->broadcast("scriptlist_changed");
#endif

    /*VRName::load(e);
    VRCameraManager::loadChildFrom(e);
    VRRenderManager::loadChildFrom(e);

    VRNetworkManager::loadChildFrom(e);
    VRBackground::loadChildFrom(e);
    VRNavigator::loadChildFrom(e);
    VRMaterialManager::loadChildFrom(e);
    semanticManager->loadChildFrom(e);

    VRCameraManager::CMsetup();
    VRRenderManager::update();
    VRNetworkManager::update();
    VRBackground::update();
    VRNavigator::update();
    VRMaterialManager::update();
    semanticManager->update();*/
}

VRSemanticManagerPtr VRScene::getSemanticManager() { return semanticManager; }

OSG_END_NAMESPACE;
