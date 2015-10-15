#include "VRMainInterface.h"
#include "core/setup/devices/VRMobile.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneLoader.h"

#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;

VRMainInterface::VRMainInterface() {
    mobile = new VRMobile(5501);
    VRSignal* sig = mobile->addSignal(0,1);
    sig->add( new VRDevCb( "VRMainInterface_on_scene_clicked", boost::bind(&VRMainInterface::on_scene_clicked, this, _1) ) );
    update();
}

VRMainInterface::~VRMainInterface() {
    delete mobile;
}

VRMainInterface* VRMainInterface::get() {
    static VRMainInterface* instance = new VRMainInterface();
    return instance;
}

void VRMainInterface::on_scene_clicked(VRDevice* dev) {
    string path = dev->getMessage();
    cout << "switch to scene " << path << endl;
    VRSceneManager::get()->loadScene(path);
    update();
}

void VRMainInterface::update() {
    page = "<html><body>\n";

    page += "<script>\n";
    page += "function get(b,s,m) {\n";
    page += "    var xmlHttp = new XMLHttpRequest();\n";
    page += "    xmlHttp.open( \"GET\", document.URL+'?button='+b+'&state='+s+'&message='+m, true );\n";
    page += "    xmlHttp.send( null );\n";
    //page += "    location.reload();\n";
    page += "}\n";
    page += "</script>\n";

    page += "<h1>Status</h1>";
    auto scene = VRSceneManager::getCurrent();
    if (scene) {
        page += "<h3> active scene: " + scene->getName() + "</h3>";
        page += "<h3> active setup: " + scene->getName() + "</h3>";
    }

    page += "<h1>Scenes</h1>";
    page += "<h2>Favorites:</h2>";
    for (auto s : VRSceneManager::get()->getFavoritePaths()) page += "<button onClick='get(0,1,\"" + s + "\")'>" + s + "</button><br>";
    page += "<h2>Examples:</h2>";
    for (auto s : VRSceneManager::get()->getExamplePaths() ) page += "<button onClick='get(0,1,\"" + s + "\")'>" + s + "</button><br>";

    page += "</body></html>";
    mobile->addWebSite("", page);
}

OSG_END_NAMESPACE;
