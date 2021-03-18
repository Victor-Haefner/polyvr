#include "VRSetup.h"
#include "VRSetupManager.h"
#include "VRNetwork.h"
#include "windows/VRWindow.h"
#include "windows/VRView.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#ifndef WITHOUT_GTK
#include "core/setup/windows/VRGtkWindow.h"
#endif
#include "core/utils/toString.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRProgress.h"
#include "core/utils/xml.h"
#include "core/utils/system/VRSystem.h"
#include "core/networking/VRPing.h"

#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scripting/VRScript.h"

#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGVisitSubTree.h>

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

    setup_layer = VRVisualLayer::getLayer("Setup", "setup.png",1);
    stats_layer = VRVisualLayer::getLayer("Statistics", "stats.png",1);
    layer_setup_toggle = VRFunction<bool>::create("showSetup", bind(&VRSetup::showSetup, this, _1) );
    layer_stats_toggle = VRFunction<bool>::create("showStats", bind(&VRSetup::showStats, this, _1) );
    setup_layer->setCallback( layer_setup_toggle );
    stats_layer->setCallback( layer_stats_toggle );

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

VRScriptPtr VRSetup::addScript(string name) { auto s = VRScript::create(name); scripts[s->getName()] = s; return s; }
VRScriptPtr VRSetup::getScript(string name) { return scripts[name]; }
map<string, VRScriptPtr> VRSetup::getScripts() { return scripts; }

void setLoadingLights(int dev, int light, float R, float G, float B) {
    //cout << " !!! setLoadingLights " << dev << " " << light << " " << R << " " << G << " " << B << endl;
    static VRSocket s("cave_lights");
    string data = "device="+toString(dev);
    if (light >= 0) data += "&light="+toString(light);
    data += "&R="+toString(R)+"&G="+toString(G)+"&B="+toString(B);
    s.sendHTTPGet("http://192.168.100.55:8000/cavelights?"+data);
}

void updateLoadingLights(int p) {
    //cout << "updateLoadingLights " << p << endl;
    static float lastT = 0;
    if (p == 100) { setLoadingLights(0, -1, 0.5,0.5,0.5); lastT = 0; return; }

    float t = p*0.01;
    int dev_0 = 1+floor(lastT*3);
    int dev_t = 1+floor(t*3);
    int light_0 = floor((lastT*3-floor(lastT*3))*5);
    int light_t = floor((t*3-floor(t*3))*5);
    for (int d=dev_0; d<=dev_t; d++) {
        for (int l=light_0; l<=light_t; l++) {
            setLoadingLights(d, l, 0,1,0);
        }
    }
    lastT = t;
}

void VRSetup::setupLESCCAVELights(VRScenePtr scene) {
    VRPing ping;
    bool lightAvailable = ping.start("192.168.100.55", "8000", 1);

    string core = "\n print 'YAAAAAAY'";
    //cout << "VRSetup::setupLESCCAVELights A\n";
    string name = "setCaveLoadingProgress";
    //auto s = newScript(name, core);

    //triggerScript(name);
    //cout << "VRSetup::setupLESCCAVELights B\n";

    static auto fkt1 = VRAnimCb::create("setup loading lights cb", bind(updateLoadingLights, _1));
    auto p = scene->getLoadingProgress();
    if (lightAvailable) {
        p->setCallback(fkt1);
        setLoadingLights(1,-1,1,0,0);
        setLoadingLights(2,-1,1,0,0);
        setLoadingLights(3,-1,1,0,0);
        setLoadingLights(4,-1,0,0,0);
    }// else p->setup("scene loading progress", 100, VRProgress::CONSOLE_M);
}

void VRSetup::updateTracking() {
#ifndef WITHOUT_ART
    ART::applyEvents();
#endif
#ifndef WITHOUT_VRPN
    VRPN::update();
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
    for (int ID : getARTDevices()) {
        ART_devicePtr dev = getARTDevice(ID);
        if (dev->ent && dev->ent->getName() == t) return dev->ent;
    }
#endif

#ifndef WITHOUT_VRPN
    for (int ID : getVRPNTrackerIDs()) {
        VRPN_devicePtr dev = getVRPNTracker(ID);
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
    ART::save(trackingARTN);
#endif
#ifndef WITHOUT_VRPN
    VRPN::save(trackingVRPNN);
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
    if (trackingARTN) ART::load(trackingARTN);
#endif
#ifndef WITHOUT_VRPN
    if (trackingVRPNN) VRPN::load(trackingVRPNN);
#endif
    if (deviceN) VRDeviceManager::load(deviceN);
    if (displayN) VRWindowManager::load(displayN);
    if (networkN) network->load(networkN);
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

