#include "VRSetup.h"
#include "VRSetupManager.h"
#include "VRNetwork.h"
#include "windows/VRWindow.h"
#include "windows/VRView.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/utils/toString.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRVisualLayer.h"
#include "core/utils/VRProgress.h"
#include "core/networking/VRPing.h"
#include "core/setup/tracking/Vive.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"
#include "core/scripting/VRScript.h"
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>

#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGNameAttachment.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSetup::VRSetup(string name) {
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

    vive = shared_ptr<Vive>( new Vive() );

    setup_layer = VRVisualLayer::getLayer("Setup", "setup.png",1);
    stats_layer = VRVisualLayer::getLayer("Statistics", "stats.png",1);
    layer_setup_toggle = VRFunction<bool>::create("showSetup", boost::bind(&VRSetup::showSetup, this, _1) );
    layer_stats_toggle = VRFunction<bool>::create("showStats", boost::bind(&VRSetup::showStats, this, _1) );
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
    auto w = getEditorWindow();
    for (auto v : w->getViews()) v->showStats(b);
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

    static auto fkt1 = VRAnimCb::create("setup loading lights cb", boost::bind(updateLoadingLights, _1));
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
    ART::applyEvents();
    VRPN::update();
    vive->update();
    for (auto view : getViews()) view->updateMirror();
}

VRNetworkPtr VRSetup::getNetwork() { return network; }

//parser callback for the xml scene import
void VRSetup::parseSetup(xmlpp::Element* setup) {
    ;
}

void VRSetup::processOptions() {
    cfgfile = VROptions::get()->getOption<string>("setupCfg");

    static xmlpp::DomParser parser;
    parser.parse_file(cfgfile);
    parseSetup(parser.get_document()->get_root_node());
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

    //scene->initDevices();
}

void VRSetup::addObject(VRObjectPtr o) { real_root->addChild(o); }

void VRSetup::showSetup(bool b) { // TODO
    showViewportGeos(b);
}

VRTransformPtr VRSetup::getUser() { return user; }
VRTransformPtr VRSetup::getRoot() { return real_root; }

VRTransformPtr VRSetup::getTracker(string t) {
    for (int ID : getARTDevices()) {
        ART_devicePtr dev = getARTDevice(ID);
        if (dev->ent && dev->ent->getName() == t) return dev->ent;
    }

    for (int ID : getVRPNTrackerIDs()) {
        VRPN_devicePtr dev = getVRPNTracker(ID);
        if (dev->getName() == t) return dev->getBeacon();
    }

    return 0;
}

xmlpp::Element* VRSetup::getElementChild(xmlpp::Element* e, string name) {
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        if (el->get_name() == name) return el;
    }
    return 0;
}

void VRSetup::printOSG() {
    cout << "Setup " << endl;
    string name = "Unnamed";
    for (auto win : getWindows()) {
        VRWindowPtr w = win.second;
        WindowMTRecPtr osgw = w->getOSGWindow();
        cout << "Window " << win.first << " " << osgw->getTypeName() << endl;
        int N = osgw->getMFPort()->size();

        for (int i=0; i<N; i++) {
            ViewportRefPtr view = osgw->getPort(i);
            name = OSG::getName(view) ? OSG::getName(view) : "Unnamed";
            cout << "View " << name << " " << view->getTypeName() << endl;
        }
    }
}

Vec3d VRSetup::getDisplaysOffset() { return globalOffset; }
void VRSetup::setDisplaysOffset(Vec3d o) {
    globalOffset = o;
    for (auto v : getViews()) v->setOffset(o);
}

void VRSetup::save(string file) {
    xmlpp::Document doc;
    xmlpp::Element* setupN = doc.create_root_node("Setup", "", "VRF"); //name, ns_uri, ns_prefix
    xmlpp::Element* displayN = setupN->add_child("Displays");
    xmlpp::Element* deviceN = setupN->add_child("Devices");
    xmlpp::Element* trackingARTN = setupN->add_child("TrackingART");
    xmlpp::Element* trackingVRPNN = setupN->add_child("TrackingVRPN");
    xmlpp::Element* networkN = setupN->add_child("Network");
    xmlpp::Element* scriptN = setupN->add_child("Scripts");

    VRWindowManager::save(displayN);
    VRDeviceManager::save(deviceN);
    ART::save(trackingARTN);
    VRPN::save(trackingVRPNN);
    network->save(networkN);
    displayN->set_attribute("globalOffset", toString(globalOffset).c_str());
    for (auto s : scripts) {
        auto e = s.second->saveUnder(scriptN);
        s.second->save(e);
    }

    if (file == "") file = path;
    if (file != "") doc.write_to_file_formatted(file);
}

void VRSetup::load(string file) {
    cout << " load setup " << file << endl;
    path = file;
    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(file.c_str());

    xmlpp::Node* n = parser.get_document()->get_root_node();
    xmlpp::Element* setupN = dynamic_cast<xmlpp::Element*>(n);
    xmlpp::Element* displayN = getElementChild(setupN, "Displays");
    xmlpp::Element* deviceN = getElementChild(setupN, "Devices");
    xmlpp::Element* trackingARTN = getElementChild(setupN, "TrackingART");
    xmlpp::Element* trackingVRPNN = getElementChild(setupN, "TrackingVRPN");
    xmlpp::Element* networkN = getElementChild(setupN, "Network");
    xmlpp::Element* scriptN = getElementChild(setupN, "Scripts");

    if (trackingARTN) ART::load(trackingARTN);
    if (trackingVRPNN) VRPN::load(trackingVRPNN);
    if (deviceN) VRDeviceManager::load(deviceN);
    if (displayN) VRWindowManager::load(displayN);
    if (networkN) network->load(networkN);
    for (auto el : getChildren(scriptN)) {
        auto s = VRScript::create("tmp");
        s->load(el);
        scripts[s->getName()] = s;
    }

    if (displayN && displayN->get_attribute("globalOffset")) {
        toValue( displayN->get_attribute("globalOffset")->get_value(), globalOffset );
        setDisplaysOffset(globalOffset);
    }
}

OSG_END_NAMESPACE;
