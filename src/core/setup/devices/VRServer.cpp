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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace OSG;


VRServer::VRServer(int port) : VRDevice("server") {
    addBeacon();
#ifndef __EMSCRIPTEN__
    soc = VRSceneManager::get()->getSocket(port);
    this->port = soc->getPort();
    cb = new VRHTTP_cb( "Mobile_server_callback", bind(&VRServer::callback, this, placeholders::_1) );
    soc->setHTTPCallback(cb);
    soc->setType("http receive");
#endif
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
    string message;

    if (args->websocket) {
        button = args->ws_id;
        message = args->ws_data;
        setMessage(message);
        if (args->ws_data.size() == 0) return;
    } else {
        if (args->params->count("button") == 0) { /*cout << "VRServer::callback warning, no button passed\n";*/ return; }
        if (args->params->count("state") == 0) { /*cout << "VRServer::callback warning, no state passed\n";*/ return; }
        if (args->params->count("message")) {
            message = (*args->params)["message"];
	    setMessage(message);
        }

        button = toInt((*args->params)["button"]);
        state = toInt((*args->params)["state"]);
    }

    VRLog::log("net", "VRServer::callback button: " + toString(button) + " state: " + toString(state) + "\n");
    change_button(button, state);
}

void VRServer::handleMessage(const string& m, int button, int state) {
    setMessage(m);
    change_button(button, state);
}

map<string, vector<int>> VRServer::getClients() {
    map<string, vector<int>> res;
    if (soc) res = soc->getClients();
    return res;
}

int VRServer::openWebSocket(string address, string protocols) {
    if (!soc) return -1;
    return soc->openWebSocket(address, protocols);
}

void VRServer::answer(int id, string msg) {
    if (soc) soc->answerWebSocket(id, msg);
#ifdef __EMSCRIPTEN__
    EM_ASM({
        var msg = Module.UTF8ToString($0);
        var cID = $1;
        sendToClient(cID, msg);
    }, msg.c_str(), id);
#endif
}

void VRServer::clearSignals() {
    VRDevice::clearSignals();

    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRServer::setPort(int port) { this->port = port; if (soc) soc->setPort(port); }
int VRServer::getPort() { return soc ? soc->getPort() : port; }

void VRServer::addCallback(string path, VRServerCbPtr cb) {
    callbacks[path] = cb;
    if (soc) soc->addHTTPCallback(path, cb);
}

void VRServer::remCallback(string path) {
    if (callbacks.count(path)) callbacks.erase(path);
    if (soc) soc->remHTTPCallback(path);
}

void VRServer::updateMobilePage() {
    //return; //TODO: this induces a segfault when closing PolyVR
    string page = "<html><body>";
    for (auto w : websites) page += "<a href=\"" + w.first + "\">" + w.first + "</a>";
    page += "</body></html>";
    if (soc) soc->addHTTPPage(getName(), page);

    //if (websites.size()) soc->addHTTPPage(getName(), page);
    //else soc->remHTTPPage(getName());
}

void VRServer::addWebSite(string uri, string website) {
    websites[uri] = website;
    if (soc) soc->addHTTPPage(uri, website);
    updateMobilePage();
}

void VRServer::remWebSite(string uri) {
    if (websites.count(uri)) websites.erase(uri);
    if (soc) soc->remHTTPPage(uri);
    updateMobilePage();
}

void VRServer::updateClients(string path) {
#ifndef WITHOUT_CEF
    CEF::reloadScripts(path);
#endif
}

