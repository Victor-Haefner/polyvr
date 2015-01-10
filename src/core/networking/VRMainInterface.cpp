#include "VRMainInterface.h"
#include "core/setup/devices/VRMobile.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"

OSG_BEGIN_NAMESPACE;

VRMainInterface::VRMainInterface() {
    mobile = new VRMobile(5501);
    update();
}

VRMainInterface::~VRMainInterface() {}

VRMainInterface* VRMainInterface::get() {
    static VRMainInterface* instance = new VRMainInterface();
    return instance;
}

void VRMainInterface::update() {
    // get a list of scenes (TODO: move the scene management from the demo gui to the scene manager)
    // list all the scenes on this site
    vector<string> scenes;
    scenes.push_back("Scene1");
    scenes.push_back("Scene2");
    scenes.push_back("Scene3");

    page = "<html><body>";

    page += "<script>";
    page += "function get(b,s) {";
    page += "    var xmlHttp = new XMLHttpRequest();";
    page += "    xmlHttp.open( \"GET\", document.URL+'?button='+b+'&state='+s, true );";
    page += "    xmlHttp.send( null );";
    page += "}";
    page += "</script>";

    page += "Scenes:\n\n";
    for (auto s : scenes) page += "<button onClick='get(0,1)'>" + s + "</button>";

    page += "</body></html>";
    mobile->addWebSite("", page);
}

OSG_END_NAMESPACE;
