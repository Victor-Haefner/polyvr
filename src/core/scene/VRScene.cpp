#include "VRScene.h"
#include "core/setup/devices/VRFlystick.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRMultiTouch.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/setup/devices/VRHaptic.h"
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
#include <libxml++/nodes/element.h>

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
    addUpdateFkt(updatePhysObjectsFkt);

    physicsThreadID = initThread(updatePhysicsFkt, "physics", true, 0);
    loadingTimeCb = VRUpdateCb::create("loadingTimeCb", boost::bind(&VRScene::recLoadingTime, this));

    initDevices();
    VRMaterial::getDefault()->resetDefault();

    referentials_layer = VRVisualLayer::getLayer("Referentials", "refs.png", 1);
    cameras_layer = VRVisualLayer::getLayer("Cameras", "cameras.png", 1);
    lights_layer = VRVisualLayer::getLayer("Lights", "lights.png", 1);

    layer_ref_toggle = VRFunction<bool>::create("showReferentials", boost::bind(&VRScene::showReferentials, this, _1, (VRObjectPtr)0) );
    layer_cam_toggle = VRFunction<bool>::create("showCameras", boost::bind(&VRScene::showCameras, this, _1) );
    layer_light_toggle = VRFunction<bool>::create("showLights", boost::bind(&VRScene::showLights, this, _1) );
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
    VRSetupPtr setup = VRSetup::getCurrent();
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

VRObjectPtr VRScene::get(string name) {
    VRObjectPtr o = 0;
    o = root->find(name);
    return o;
}

void VRScene::setActiveCamera(string camname) {
    cout << "set active camera: " << camname << endl;
    setMActiveCamera(camname);
    VRSetupPtr setup = VRSetup::getCurrent();

    // TODO: refactor the following workaround
    VRCameraPtr cam = getActiveCamera();
    if (cam == 0) return;

    VRMousePtr mouse = dynamic_pointer_cast<VRMouse>( setup->getDevice("mouse") );
    VRMultiTouchPtr multitouch = dynamic_pointer_cast<VRMultiTouch>( setup->getDevice("multitouch") );
    VRFlystickPtr flystick = dynamic_pointer_cast<VRFlystick>( setup->getDevice("flystick") );
    VRDevicePtr razer = setup->getDevice("vrpn_device");
    VRServerPtr server1 = dynamic_pointer_cast<VRServer>( setup->getDevice("server1") );

    if (mouse) {
        mouse->setTarget(cam);
        mouse->setCamera(cam);
    }

    if (multitouch) {
        multitouch->setTarget(cam);
        multitouch->setCamera(cam);
    }

    if (flystick) flystick->setTarget(cam);
    if (razer) razer->setTarget(cam);
    if (server1) server1->setTarget(cam);

    setup->setViewCamera(cam, -1);
    if (cam->getAcceptRoot()) setup->getRoot()->switchParent(cam);
}

VRObjectPtr VRScene::getRoot() { return root; }
//VRObjectPtr VRScene::getSystemRoot() { return root_system; }

void VRScene::printTree() { root->printTree(); }

void VRScene::showReferentials(bool b, VRObjectPtr o) {
    if (o == 0) o = root;

    VRTransformPtr t = 0;
    if (o->hasTag("transform")) t = static_pointer_cast<VRTransform>(o);
    if (t) t->showCoordAxis(b);

    for (uint i=0; i<o->getChildrenCount(); i++) showReferentials(b, o->getChild(i));
}

void VRScene::showLights(bool b) { for (auto be : VRLightBeacon::getAll()) be.lock()->showLightGeo(b); }
void VRScene::showCameras(bool b) { for (auto c : VRCamera::getAll()) if (auto sp = c.lock()) sp->showCamGeo(b); }

void VRScene::update() {
    //Vec3d min,max;
    //root->getNode()->updateVolume();
    //root->getNode()->getVolume().getBounds( min, max );
    ThreadManagerUpdate();
    updateCallbacks();
    VRTransform::updateConstraints();
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

bool exists(string p) {
    struct stat st;
    return (stat(p.c_str(),&st) == 0);
}

void mkDir(string path) {
    if (!exists(path)) mkdir(path.c_str(), ACCESSPERMS);
}

void mkPath(string path) {
    auto dirs = splitString(path, '/');
    path = "";
    for (uint i=1; i<dirs.size(); i++) {
        path += "/"+dirs[i];
        mkDir(path);
    }
}

void VRScene::saveScene(xmlpp::Element* e) {
    if (e == 0) return;
    VRName::save(e);
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

void VRScene::loadScene(xmlpp::Element* e) {
    if (e == 0) return;

    string d = getWorkdir() + "/.local_"+getFileName()+"/stats";
    if (exists(d)) {
        ifstream stats(d);
        stats >> loadingTime;
        stats.close();
    }

    loadingTimer.start();
    loadingProgressThreadCb = VRFunction< VRThreadWeakPtr >::create( "loading progress thread", boost::bind(&VRScene::updateLoadingProgress, this, _1) );
    loadingProgressThread = VRSceneManager::get()->initThread(loadingProgressThreadCb, "loading progress thread", true, 1);

    VRName::load(e);
    VRRenderManager::loadChildFrom(e);
    VRScriptManager::loadChildFrom(e);
    VRNetworkManager::loadChildFrom(e);
    VRBackground::loadChildFrom(e);
    VRNavigator::loadChildFrom(e);
    VRMaterialManager::loadChildFrom(e);
    semanticManager->loadChildFrom(e);

    VRRenderManager::update();
    VRScriptManager::update();
    VRNetworkManager::update();
    VRBackground::update();
    VRNavigator::update();
    VRMaterialManager::update();
    semanticManager->update();

    queueJob(loadingTimeCb, 0, 2);
}

VRSemanticManagerPtr VRScene::getSemanticManager() { return semanticManager; }

OSG_END_NAMESPACE;
