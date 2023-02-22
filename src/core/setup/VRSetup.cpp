#include "VRSetup.h"
#include "VRSetupManager.h"
#include "VRNetwork.h"
#include "windows/VRWindow.h"
#include "windows/VRView.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#ifndef WITHOUT_GTK
#include "core/setup/windows/VRGtkWindow.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRKeyboard.h"
#endif
#include "core/utils/toString.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRProgress.h"
#include "core/utils/xml.h"
#include "core/utils/system/VRSystem.h"
#ifndef __EMSCRIPTEN__
#include "core/networking/VRPing.h"
#endif

#include "tracking/VRPN.h"
#include "tracking/ART.h"

#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scripting/VRScript.h"

#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGVisitSubTree.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace OSG;


VRSetup::VRSetup(string name) {
    cout << "Init VRSetup " << name << endl;
    setName(name);
    real_root = VRTransform::create("VR Setup");
    setup_cam = VRCamera::create("Setup", false);
    setViewAnchor(real_root);
    setDeviceRoot(real_root);
    real_root->addAttachment("treeviewNotDragable", 0);
    real_root->setPersistency(0);
    real_root->addChild(setup_cam);
    setup_cam->setAcceptRoot(false);
    user = 0;
    tracking = "None";

#ifndef WITHOUT_ART
    art = ART::create();
#endif
#ifndef WITHOUT_VRPN
    vrpn = VRPN::create();
#endif

    setup_layer = VRVisualLayer::getLayer("Setup", "setup.png", 1);
    stats_layer = VRVisualLayer::getLayer("Statistics", "stats.png", 1);
    stencil_layer = VRVisualLayer::getLayer("Stencil", "stencil.png", 1);
    pause_layer = VRVisualLayer::getLayer("Pause rendering", "gtk-media-pause", 1);
    layer_setup_toggle = VRFunction<bool>::create("showSetup", bind(&VRSetup::showSetup, this, _1) );
    layer_stats_toggle = VRFunction<bool>::create("showStats", bind(&VRSetup::showStats, this, _1) );
    layer_stencil_toggle = VRFunction<bool>::create("showStencil", bind(&VRSetup::showStencil, this, _1) );
    layer_pause_toggle = VRFunction<bool>::create("togglePause", bind(&VRSetup::togglePause, this, _1) );
    setup_layer->setCallback( layer_setup_toggle );
    stats_layer->setCallback( layer_stats_toggle );
    stencil_layer->setCallback( layer_stencil_toggle );
    pause_layer->setCallback( layer_pause_toggle );

    network = VRNetworkPtr( new VRNetwork() );
}

VRSetup::~VRSetup() {
    cout << "~VRSetup " << name << endl;
}

VRSetupPtr VRSetup::create(string name) { return VRSetupPtr(new VRSetup(name)); }
VRSetupPtr VRSetup::getCurrent() { return VRSetupManager::get()->getCurrent(); }

void VRSetup::showStats(bool b) {
#ifndef WITHOUT_GTK
    auto w = getEditorWindow();
    for (auto v : w->getViews()) v->showStats(b);
#endif
}

void VRSetup::showStencil(bool b) {
    auto s = VRScene::getCurrent();
    if (s) s->setStencil(b);
}

void VRSetup::togglePause(bool b) {
    pauseRendering(b);
}

ARTPtr VRSetup::getART() { return art; }
VRPNPtr VRSetup::getVRPN() { return vrpn; }

VRScriptPtr VRSetup::addScript(string name) { auto s = VRScript::create(name); scripts[s->getName()] = s; return s; }
VRScriptPtr VRSetup::getScript(string name) { return scripts[name]; }
map<string, VRScriptPtr> VRSetup::getScripts() { return scripts; }

void VRSetup::updateGtkDevices() {
#ifndef WITHOUT_GTK
    for (auto dev : getDevices()) {
        auto m = dynamic_pointer_cast<VRMouse>(dev.second);
        auto k = dynamic_pointer_cast<VRKeyboard>(dev.second);
        if (m) m->applyEvents();
        if (k) k->applyEvents();
    }
#endif
}

void VRSetup::updateTracking() {
#ifndef WITHOUT_ART
    if (art) art->applyEvents();
#endif
#ifndef WITHOUT_VRPN
    if (vrpn) vrpn->update();
#endif
    for (auto view : getViews()) view->updateMirror();
}

VRNetworkPtr VRSetup::getNetwork() { return network; }

//parser callback for the xml scene import
void VRSetup::parseSetup(XMLElementPtr setup) {
    ;
}

void VRSetup::processOptions() {
    cfgfile = VROptions::get()->getOption<string>("setupCfg");
    static XML xml;
    xml.read(cfgfile);
    parseSetup(xml.getRoot());
}

void VRSetup::setScene(VRScenePtr scene) {
    if (scene == 0) return;
    VRCameraPtr cam = scene->getActiveCamera();
    if (cam == 0) return;
    setViewRoot(scene->getRoot(), -1);
    setViewCamera(cam, -1);
    setViewBackground(scene->getBackground());

    for (auto dev : getDevices()) dev.second->setCamera(cam);
    for (auto w : getWindows()) w.second->setContent(true);
    for (auto s : scripts) scene->addScript(s.second);
#ifndef WITHOUT_GTK
    getEditorWindow()->doResize();
#endif
    //scene->initDevices();
}

void VRSetup::addObject(VRObjectPtr o) { real_root->addChild(o); }

void VRSetup::showSetup(bool b) { // TODO
    showViewportGeos(b);
}

VRTransformPtr VRSetup::getUser() { return user; }
VRTransformPtr VRSetup::getRoot() { return real_root; }

VRTransformPtr VRSetup::getTracker(string t) {
#ifndef WITHOUT_ART
    for (int ID : art->getARTDevices()) {
        ART_devicePtr dev = art->getARTDevice(ID);
        if (dev->ent && dev->ent->getName() == t) return dev->ent;
    }
#endif

#ifndef WITHOUT_VRPN
    for (int ID : vrpn->getVRPNTrackerIDs()) {
        VRPN_devicePtr dev = vrpn->getVRPNTracker(ID);
        if (dev->getName() == t) return dev->getBeacon();
    }
#endif

    return 0;
}

void VRSetup::printOSG() {
    std::function<void(Node*, string)> printOSGNode = [&](Node* node, string indent) {
        string name = OSG::getName(node) ? OSG::getName(node) : "Unnamed";
        cout << indent << "Node: " << name << " <- " << node->getCore()->getTypeName() << endl;
        for (unsigned int i=0; i < node->getNChildren(); i++) printOSGNode(node->getChild(i), indent + " ");
        if (string(node->getCore()->getTypeName()) == "VisitSubTree") {
            VisitSubTree* visitor = dynamic_cast<VisitSubTree*>( node->getCore() );
            Node* link = visitor->getSubTreeRoot();
            printOSGNode(link, indent + " ");
        }
    };

    cout << "Setup windows" << endl;
    string name = "Unnamed";
    for (auto win : getWindows()) {
        VRWindowPtr w = win.second;
        WindowMTRecPtr osgw = w->getOSGWindow();
        cout << " Window " << win.first << " " << osgw->getTypeName() << endl;
        int N = osgw->getMFPort()->size();

        for (int i=0; i<N; i++) {
            ViewportRefPtr view = osgw->getPort(i);
            name = OSG::getName(view) ? OSG::getName(view) : "Unnamed";
            cout << "  View " << name << " " << view->getTypeName() << endl;
            printOSGNode( view->getRoot(), "   " );
        }
    }
}

Vec3d VRSetup::getDisplaysOffset() { return globalOffset; }
void VRSetup::setDisplaysOffset(Vec3d o) {
    globalOffset = o;
    for (auto v : getViews()) v->setOffset(o);
}

void VRSetup::save(string file) {
    XML xml;
    XMLElementPtr setupN = xml.newRoot("Setup", "", ""); //name, ns_uri, ns_prefix
    XMLElementPtr displayN = setupN->addChild("Displays");
    XMLElementPtr deviceN = setupN->addChild("Devices");
    XMLElementPtr trackingARTN = setupN->addChild("TrackingART");
    XMLElementPtr trackingVRPNN = setupN->addChild("TrackingVRPN");
    XMLElementPtr networkN = setupN->addChild("Network");
    XMLElementPtr scriptN = setupN->addChild("Scripts");

    VRWindowManager::save(displayN);
    VRDeviceManager::save(deviceN);
#ifndef WITHOUT_ART
    art->save(trackingARTN);
#endif
#ifndef WITHOUT_VRPN
    vrpn->save(trackingVRPNN);
#endif
    network->save(networkN);
    displayN->setAttribute("globalOffset", toString(globalOffset).c_str());
    for (auto s : scripts) {
        auto e = s.second->saveUnder(scriptN);
        s.second->save(e);
    }

    if (file != "" && file != path) {
        path = file;
        setName( getFileName(file, true) );
    }
    xml.write(path);
}

void VRSetup::load(string file) {
    cout << " load setup " << file << endl;
    path = file;
    XML xml;
    xml.read(file, false);

    XMLElementPtr setupN = xml.getRoot();
    XMLElementPtr displayN = setupN->getChild("Displays");
    XMLElementPtr deviceN = setupN->getChild("Devices");
    XMLElementPtr trackingARTN = setupN->getChild("TrackingART");
    XMLElementPtr trackingVRPNN = setupN->getChild("TrackingVRPN");
    XMLElementPtr networkN = setupN->getChild("Network");
    XMLElementPtr scriptN = setupN->getChild("Scripts");

#ifndef WITHOUT_ART
    if (trackingARTN && art) art->load(trackingARTN);
#endif
#ifndef WITHOUT_VRPN
    if (trackingVRPNN && vrpn) vrpn->load(trackingVRPNN);
#endif
    if (deviceN) VRDeviceManager::load(deviceN);
    if (displayN) VRWindowManager::load(displayN);
    if (networkN) {
        network->load(networkN);
        network->joinInitThreads();
    }
    for (auto el : scriptN->getChildren()) {
        auto s = VRScript::create("tmp");
        s->load(el);
        scripts[s->getName()] = s;
    }

    if (displayN && displayN->hasAttribute("globalOffset")) {
        toValue( displayN->getAttribute("globalOffset"), globalOffset );
        setDisplaysOffset(globalOffset);
    }
    cout << " setup loaded" << endl;
}

void VRSetup::makeTestCube() {
    static bool done = false;
    if (done) return;
    done = true;

    auto cube = VRGeometry::create("testCube");
    cube->setPrimitive("Box 0.1 0.1 0.1 1 1 1");
    cube->setFrom(Vec3d(0.1,0.1,-1));
    cube->setPickable(true);
    getRoot()->addChild(cube);
}

void VRSetup::sendToBrowser(const string& msg) {
#ifdef __EMSCRIPTEN__
    EM_ASM_INT({
        var msg = Module.UTF8ToString($0);
	//console.log("sendToBrowser "+msg);
	if (typeof handleFromPolyVR === "function") {
	    handleFromPolyVR(msg);
	}
    }, msg.c_str());
#endif
}

