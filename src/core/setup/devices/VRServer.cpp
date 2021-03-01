#include "VRServer.h"
#include "VRSignal.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/networking/VRNetworkManager.h"
#include "core/networking/VRSocket.h"
#include "core/objects/VRTransform.h"
#ifndef WITHOUT_CEF
#include "addons/CEF/CEF.h"
#endif
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRLogger.h"

using namespace OSG;


VRServer::VRServer(int port) : VRDevice("server") {
    addBeacon();
    soc = VRSceneManager::get()->getSocket(port);
    this->port = port;
    cb = new VRHTTP_cb( "Mobile_server_callback", bind(&VRServer::callback, this, placeholders::_1) );
    soc->setHTTPCallback(cb);
    soc->setType("http receive");
}

VRServer::~VRServer() {
    //cout << "~VRServer " << getName() << endl;
}

VRServerPtr VRServer::create(int p) {
    auto d = VRServerPtr(new VRServer(p));
    d->initIntersect(d);
    d->clearSignals();
    return d;
}

VRServerPtr VRServer::ptr() { return static_pointer_cast<VRServer>( shared_from_this() ); }

void VRServer::callback(void* _args) { // TODO: implement generic button trigger of device etc..
    //args->print();
	HTTP_args* args = (HTTP_args*)_args;

	int button, state;
	button = state = 0;

    if (args->websocket) {
        button = args->ws_id;
        state = 0;
        setMessage(args->ws_data);
        if (args->ws_data.size() == 0) return;
    } else {
        if (args->params->count("button") == 0) { /*cout << "VRServer::callback warning, no button passed\n";*/ return; }
        if (args->params->count("state") == 0) { /*cout << "VRServer::callback warning, no state passed\n";*/ return; }
        if (args->params->count("message")) setMessage((*args->params)["message"]);

        button = toInt((*args->params)["button"]);
        state = toInt((*args->params)["state"]);
    }

    VRLog::log("net", "VRServer::callback button: " + toString(button) + " state: " + toString(state) + "\n");
    change_button(button, state);
}

map<string, vector<int>> VRServer::getClients() {
    return soc->getClients();
}

int VRServer::openWebSocket(string address, string protocols) {
    return soc->openWebSocket(address, protocols);
}

void VRServer::answerWebSocket(int id, string msg) {
    soc->answerWebSocket(id, msg);
}

void VRServer::clearSignals() {
    VRDevice::clearSignals();

    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRServer::setPort(int port) { this->port = port; soc->setPort(port); }
int VRServer::getPort() { return port; }

void VRServer::addCallback(string path, VRServerCbPtr cb) {
    callbacks[path] = cb;
    soc->addHTTPCallback(path, cb);
}

void VRServer::remCallback(string path) {
    if (callbacks.count(path)) callbacks.erase(path);
    soc->remHTTPCallback(path);
}

void VRServer::updateMobilePage() {
    //return; //TODO: this induces a segfault when closing PolyVR
    string page = "<html><body>";
    for (auto w : websites) page += "<a href=\"" + w.first + "\">" + w.first + "</a>";
    page += "</body></html>";
    soc->addHTTPPage(getName(), page);

    //if (websites.size()) soc->addHTTPPage(getName(), page);
    //else soc->remHTTPPage(getName());
}

void VRServer::addWebSite(string uri, string website) {
    websites[uri] = website;
    soc->addHTTPPage(uri, website);
    updateMobilePage();
}

void VRServer::remWebSite(string uri) {
    if (websites.count(uri)) websites.erase(uri);
    soc->remHTTPPage(uri);
    updateMobilePage();
}

void VRServer::updateClients(string path) {
#ifndef WITHOUT_CEF
    CEF::reloadScripts(path);
#endif
}

