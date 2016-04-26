#include "VRSetup.h"
#include "VRNetwork.h"
#include "windows/VRWindow.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRVisualLayer.h"
#include "core/setup/devices/VRMouse.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"
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

    setup_layer = new VRVisualLayer("Setup", "setup.png");
    stats_layer = new VRVisualLayer("Statistics", "stats.png");
    layer_setup_toggle = VRFunction<bool>::create("showSetup", boost::bind(&VRSetup::showSetup, this, _1) );
    layer_stats_toggle = VRFunction<bool>::create("showStats", boost::bind(&VRViewManager::showViewStats, this, 0, _1) );
    setup_layer->setCallback( layer_setup_toggle );
    stats_layer->setCallback( layer_stats_toggle );

    network = VRNetworkPtr( new VRNetwork() );
}

VRSetup::~VRSetup() {
    cout << "~VRSetup " << name << endl;
    delete setup_layer;
    delete stats_layer;
}

VRSetupPtr VRSetup::create(string name) { return VRSetupPtr(new VRSetup(name)); }

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
    setViewRoot(scene->getSystemRoot(), -1);
    setViewCamera(cam, -1);

    VRMouse* mouse = (VRMouse*)getDevice("mouse");
    if (mouse && cam) mouse->setCamera(cam);

    setViewBackground(scene->getBackground());

    for (auto w : getWindows()) w.second->setContent(true);

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
        ART_device* dev = getARTDevice(ID);
        if (dev->ent->getName() == t) return dev->ent;
    }

    for (int ID : getVRPNTrackerIDs()) {
        VRPN_device* dev = getVRPNTracker(ID);
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
        WindowRecPtr osgw = w->getOSGWindow();
        cout << "Window " << win.first << " " << osgw->getTypeName() << endl;
        int N = osgw->getMFPort()->size();

        for (int i=0; i<N; i++) {
            ViewportRefPtr view = osgw->getPort(i);
            name = OSG::getName(view) ? OSG::getName(view) : "Unnamed";
            cout << "View " << name << " " << view->getTypeName() << endl;
        }
    }
}

void VRSetup::save(string file) {
    xmlpp::Document doc;
    xmlpp::Element* setupN = doc.create_root_node("Setup", "", "VRF"); //name, ns_uri, ns_prefix
    xmlpp::Element* displayN = setupN->add_child("Displays");
    xmlpp::Element* deviceN = setupN->add_child("Devices");
    xmlpp::Element* trackingARTN = setupN->add_child("TrackingART");
    xmlpp::Element* trackingVRPNN = setupN->add_child("TrackingVRPN");
    xmlpp::Element* networkN = setupN->add_child("Network");

    VRWindowManager::save(displayN);
    VRDeviceManager::save(deviceN);
    ART::save(trackingARTN);
    VRPN::save(trackingVRPNN);
    network->save(networkN);

    doc.write_to_file_formatted(file);
}

void VRSetup::load(string file) {
    cout << " load setup " << file << endl;
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

    if (trackingARTN) ART::load(trackingARTN);
    if (trackingVRPNN) VRPN::load(trackingVRPNN);
    if (deviceN) VRDeviceManager::load(deviceN);
    if (displayN) VRWindowManager::load(displayN);
    if (networkN) network->load(networkN);
}

OSG_END_NAMESPACE;
