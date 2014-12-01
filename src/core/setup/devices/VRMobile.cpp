#include "VRMobile.h"
#include "VRSignal.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/networking/VRNetworkManager.h"
#include "core/networking/VRSocket.h"
#include "core/objects/VRTransform.h"
#include "addons/CEF/CEF.h"
//#include "addons/WebKit/VRPyWebKit.h"
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRMobile::callback(HTTP_args* args) { // TODO: implement generic button trigger of device etc..
    //args->print();

    if (args->params->count("button") == 0) return;
    if (args->params->count("state") == 0) return;

    if (args->params->count("message")) setMessage((*args->params)["message"]);

    int button = toInt((*args->params)["button"]);
    int state = toInt((*args->params)["state"]);

    change_button(button, state);
}

VRMobile::VRMobile() : VRDevice("mobile") {
    //enableAvatar("cone");
    //enableAvatar("ray");
    clearSignals();

    soc = new VRSocket("Mobile_server");
    cb = new VRHTTP_cb( "Mobile_server_callback", boost::bind(&VRMobile::callback, this, _1) );
    setPort(5500);
}

void VRMobile::clearSignals() {
    VRDevice::clearSignals();

    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon(), 0) );
}

void VRMobile::setPort(int port) { this->port = port; init(); }
int VRMobile::getPort() { return port; }

void VRMobile::init() {
    string page;

    page += "<html><body>";
    page += "<script>";
    page += "function httpGet(b,s) {";
    page += "    var xmlHttp = new XMLHttpRequest();";
    page += "    xmlHttp.open( \"GET\", document.URL+'?button='+b+'&state='+s, true );";
    page += "    xmlHttp.send( null );";
    page += "}";
    page += "</script>";
    page += "<table>";
    page += "<button type='button' ontouchstart='httpGet(2,1)' ontouchend='httpGet(2,0)' onmousedown='httpGet(2,1)' onmouseup='httpGet(2,0)' style='width:50%;height:50%;background-color:#eeeeee;'>Left</button>";
    page += "<button type='button' ontouchstart='httpGet(0,1)' ontouchend='httpGet(0,0)' onmousedown='httpGet(0,1)' onmouseup='httpGet(0,0)' style='width:50%;height:50%;background-color:#33bbff;'>Walk</button>";
    page += "<button type='button' ontouchstart='httpGet(3,1)' ontouchend='httpGet(3,0)' onmousedown='httpGet(3,1)' onmouseup='httpGet(3,0)' style='width:50%;height:50%;background-color:#33bbff;'>Right</button>";
    page += "<button type='button' ontouchstart='httpGet(1,1)' ontouchend='httpGet(1,0)' onmousedown='httpGet(1,1)' onmouseup='httpGet(1,0)' style='width:50%;height:50%;background-color:#eeeeee;'>Back</button>";
    page += "</table>";
    page += "</body></html>";

    soc->setPort(port);
    soc->setCallback(cb);
    soc->addHTTPPage("nav", page);
    soc->setType("http receive");
}

void VRMobile::addWebSite(string uri, string website) {
    websites[uri] = website;
    soc->addHTTPPage(uri, website);
}

void VRMobile::remWebSite(string uri) {
    if (websites.count(uri)) websites.erase(uri);
    soc->remHTTPPage(uri);
}

void VRMobile::updateClients(string path) { CEF::reload(path); }

vector<string> VRMobile::getUris() {
    vector<string> uris;
    for (auto itr = websites.begin(); itr != websites.end(); itr++)
        uris.push_back(soc->getIP() + ":" + toString(port) + "/" + itr->first);
    return uris;
}

OSG_END_NAMESPACE;
