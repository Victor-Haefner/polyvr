#include "VRScene.h"
#include "core/setup/devices/VRFlystick.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/setup/devices/VRHaptic.h"
#include "core/setup/devices/VRMobile.h"
#include "VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRGroup.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "core/utils/VRVisualLayer.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScene::VRScene() {
    cout << "Init Scene" << endl;

    root = new VRObject("Root");
    setNameSpace("Scene");
    setName("Scene");

    icon = "Scene.png";
    setFlag("write_protected", false);

    //scene update functions
    addUpdateFkt(updateObjectsFkt, 1000);
    addUpdateFkt(updateAnimationsFkt);
    addUpdateFkt(updatePhysObjectsFkt);

    physicsThreadID = initThread(updatePhysicsFkt, "physics", true, 0);

    initDevices();
    VRMaterial::getDefault()->resetDefault();

    referentials_layer = new VRVisualLayer("Referentials", "refs.png");
    cameras_layer = new VRVisualLayer("Cameras", "cameras.png");
    lights_layer = new VRVisualLayer("Lights", "lights.png");

    referentials_layer->setCallback( new VRFunction<bool>("showReferentials", boost::bind(&VRScene::showReferentials, this, _1, (VRObject*)0) ) );
    cameras_layer->setCallback( new VRFunction<bool>("showCameras", boost::bind(&VRScene::showCameras, this, _1) ) );
    lights_layer->setCallback( new VRFunction<bool>("showLights", boost::bind(&VRScene::showLights, this, _1) ) );

    VRVisualLayer::anchorLayers(root);

    cout << " init scene done\n";
}

VRScene::~VRScene() {
    //kill physics thread
    VRThreadManager::stopThread(physicsThreadID);
    updateObjects();
    root->destroy();
    VRGroup::clearGroups();
    VRLightBeacon::getAll().clear();
    VRCamera::getAll().clear();

}

void VRScene::initDevices() { // TODO: remove this after refactoring the navigation stuff
    VRSetup* setup = VRSetupManager::getCurrent();

    VRMouse* mouse = (VRMouse*)setup->getDevice("mouse");
    VRFlystick* flystick = (VRFlystick*)setup->getDevice("flystick");
    VRDevice* razer = setup->getDevice("vrpn_device");


    if (mouse) {
        initOrbit(getActiveCamera(), mouse); //TODO, load from xml
        initWalk(getActiveCamera(), mouse);
        initOrbit2D(getActiveCamera(), mouse);
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

    setup->updateDeviceDynNodes(getRoot());

    cout << " init devices done\n";
}

void VRScene::setPath(string path) { this->path = path; }
string VRScene::getPath() { return path; }
string VRScene::getFileName() {
    return getFile().substr(0,getFile().size()-4);
}
string VRScene::getIcon() { return getFileName() + ".png"; }
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

void VRScene::add(VRObject* obj, int parentID) {
    if (obj == 0) return;

    VRObject* o = get(parentID);
    if (o != 0) o->addChild(obj);
    else root->addChild(obj);
}

void VRScene::add(NodeRecPtr n) { root->addChild(n); }

VRObject* VRScene::get(int ID) {
    if (ID == -1) return 0;
    VRObject* o = 0;
    o = root->find(ID);
    return o;
}

VRObject* VRScene::get(string name) {
    VRObject* o = 0;
    o = root->find(name);
    return o;
}

void VRScene::setActiveCamera(int i) {
    VRCameraManager::setActiveCamera(i);
    VRSetup* setup = VRSetupManager::getCurrent();

    // TODO: refactor the following workaround
    VRCamera* cam = getActiveCamera();
    if (cam == 0) return;
    cout << " set active camera to " << cam->getName() << endl;

    VRMouse* mouse = (VRMouse*)setup->getDevice("mouse");
    if (mouse) {
        mouse->setTarget(cam);
        mouse->setCamera(cam);
    }

    VRFlystick* flystick = (VRFlystick*)setup->getDevice("flystick");
    if (flystick) flystick->setTarget(cam);

    VRDevice* razer = (VRFlystick*)setup->getDevice("vrpn_device");
    if (razer) razer->setTarget(cam);

    VRMobile* mobile = (VRMobile*)VRSetupManager::getCurrent()->getDevice("mobile");
    if (mobile) mobile->setTarget(cam);

    setup->setViewCamera(cam, -1);
    if (cam->getAcceptRoot()) setup->getRoot()->switchParent(cam);
}

VRObject* VRScene::getRoot() { return root; }

void VRScene::printTree() { root->printTree(); }

void VRScene::showReferentials(bool b, VRObject* o) {
    if (o == 0) o = root;

    VRTransform* t = 0;
    if (o->hasAttachment("transform")) t = (VRTransform*)o;
    if (t) t->showCoordAxis(b);

    for (uint i=0; i<o->getChildrenCount(); i++) showReferentials(b, o->getChild(i));
}

void VRScene::showLights(bool b) {
    vector<VRLightBeacon*> beacons = VRLightBeacon::getAll();
    for (uint i=0; i<beacons.size(); i++) beacons[i]->showLightGeo(b);
}

void VRScene::showCameras(bool b) {
    vector<VRCamera*> cams = VRCamera::getAll();
    for (uint i=0; i<cams.size(); i++) cams[i]->showCamGeo(b);
}

void VRScene::update() {
    //Vec3f min,max;
    //root->getNode()->updateVolume();
    //root->getNode()->getVolume().getBounds( min, max );
    updateCallbacks();
}

xmlpp::Element* VRSceneLoader_getElementChild(xmlpp::Element* e, string name) {
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (el->get_name() == name) return el;
    }
    return 0;
}

xmlpp::Element* VRSceneLoader_getElementChild(xmlpp::Element* e, int i) {
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    int j = 0;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (i == j) return el;
        j++;
    }

    return 0;
}


void VRScene::save(xmlpp::Element* e) {
    if (e == 0) return;
    VRName::saveName(e);

    xmlpp::Element* renderN = e->add_child("Rendering");
    xmlpp::Element* scriptsN = e->add_child("Scripts");
    xmlpp::Element* protocolsN = e->add_child("Sockets");
    xmlpp::Element* backgroundN = e->add_child("Background");
    xmlpp::Element* naviN = e->add_child("Navigation");
    xmlpp::Element* matN = e->add_child("Materials");

    VRRenderManager::save(renderN);
    VRScriptManager::save(scriptsN);
    VRNetworkManager::save(protocolsN);
    VRBackground::save(backgroundN);
    VRNavigator::save(naviN);
    VRMaterialManager::save(matN);
}

void VRScene::load(xmlpp::Element* e) {
    if (e == 0) return;
    VRName::loadName(e);

    xmlpp::Element* scriptsN = VRSceneLoader_getElementChild(e, "Scripts");
    xmlpp::Element* protocolsN = VRSceneLoader_getElementChild(e, "Sockets");
    xmlpp::Element* backgroundN = VRSceneLoader_getElementChild(e, "Background");
    xmlpp::Element* renderN = VRSceneLoader_getElementChild(e, "Rendering");
    xmlpp::Element* naviN = VRSceneLoader_getElementChild(e, "Navigation");
    xmlpp::Element* matN = VRSceneLoader_getElementChild(e, "Materials");

    VRRenderManager::load(renderN);
    VRScriptManager::load(scriptsN);
    VRNetworkManager::load(protocolsN);
    VRBackground::load(backgroundN);
    VRNavigator::load(naviN);
    VRMaterialManager::load(matN);

    VRRenderManager::update();
    VRScriptManager::update();
    VRNetworkManager::update();
    VRBackground::update();
    VRNavigator::update();
    VRMaterialManager::update();
}

OSG_END_NAMESPACE;
