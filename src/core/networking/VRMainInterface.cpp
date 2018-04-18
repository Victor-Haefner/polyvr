#include "VRMainInterface.h"
#include "core/setup/devices/VRServer.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRProjectsList.h"

#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;

VRMainInterface::VRMainInterface() {
    cout << "Init VRMainInterface..";
    server = VRServer::create(5501);
    server->setName("MainInterface");
    VRSignalPtr sig = server->addSignal(0,1);
    clickCb = VRDeviceCb::create( "VRMainInterface_on_scene_clicked", boost::bind(&VRMainInterface::on_scene_clicked, this, _1) );
    sig->add( clickCb );
    reqCb = VRServerCb::create( "VRMainInterface_request_handler", boost::bind(&VRMainInterface::handleRequest, this, _1) );
    server->addCallback( "request", reqCb);
    update();
    cout << " done" << endl;
}

VRMainInterface::~VRMainInterface() {}

VRMainInterface* VRMainInterface::get() {
    static VRMainInterface* instance = new VRMainInterface();
    return instance;
}

void VRMainInterface::on_scene_clicked(VRDeviceWeakPtr d) {
    auto dev = d.lock();
    if (!dev) return;
    string path = dev->getMessage();
    cout << "switch to scene " << path << endl;
    VRSceneManager::get()->loadScene(path);
    update();
}

void start_demo_proxy(string path) {
    auto sm = VRSceneManager::get();
    sm->loadScene(path);
}

void switch_eyes_proxy(string view) {
    auto v = VRSetup::getCurrent()->getView( view );
    v->swapEyes( !v->eyesInverted() );
    VRSetup::getCurrent()->save();
}

string VRMainInterface::handleRequest(map<string, string> params) {
    if (!params.count("var")) return "";
    string var = params["var"];
    string param = params["param"];

    auto pathsToList = [&](const vector<string>& paths) {
        string res = "{ \"paths\": [";
        for (auto s : paths) {
            res += "\""+s+"\"";
            if (s != *paths.rbegin()) res += ", ";
        }
        res += "] }";
        return res;
    };

    if (var == "favorites") {
        auto favorites = VRSceneManager::get()->getFavoritePaths();
        return pathsToList( favorites->getPaths() );
    }

    if (var == "examples") {
        auto examples = VRSceneManager::get()->getExamplePaths();
        return pathsToList( examples->getPaths() );
    }

    if (var == "toggle_calib") {
        auto scene = VRScene::getCurrent();
        if (scene) {
            auto fkt = VRUpdateCb::create("toggle_calib_job", boost::bind(&VRScene::setCalib, scene.get(), int(!scene->getCalib())));
            scene->queueJob(fkt);
        }
    }

    if (var == "start") {
        auto fkt = VRUpdateCb::create("start_demo", boost::bind(start_demo_proxy, param) );
        VRSceneManager::get()->queueJob(fkt);
    }

    if (var == "toggle_view_eye") {
        auto v = VRSetup::getCurrent()->getView( param );
        if (!v) {
            string err = "Error: view " + param + " not found! could not switch eyes, known views are: ";
            for ( auto v : VRSetup::getCurrent()->getViews() ) err += "'"+v->getName()+"' ";
            return err;
        }
        auto fkt = VRUpdateCb::create("sitch_eyes", boost::bind(switch_eyes_proxy, param) );
        VRSceneManager::get()->queueJob(fkt);
    }

    return "";
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
    auto scene = VRScene::getCurrent();
    if (scene) {
        page += "<h3> active scene: " + scene->getName() + "</h3>";
        page += "<h3> active setup: " + scene->getName() + "</h3>";
    }

    page += "<h1>Scenes</h1>";
    page += "<h2>Favorites:</h2>";
    for (auto s : VRSceneManager::get()->getFavoritePaths()->getPaths()) page += "<button onClick='get(0,1,\"" + s + "\")'>" + s + "</button><br>";
    page += "<h2>Examples:</h2>";
    for (auto s : VRSceneManager::get()->getExamplePaths()->getPaths() ) page += "<button onClick='get(0,1,\"" + s + "\")'>" + s + "</button><br>";

    page += "</body></html>";
    server->addWebSite("polyvr", page);
}

OSG_END_NAMESPACE;
