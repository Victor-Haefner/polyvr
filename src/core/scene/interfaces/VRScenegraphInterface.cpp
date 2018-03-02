#include "VRScenegraphInterface.h"
#include "core/networking/VRSocket.h"
#include "core/scene/VRSceneManager.h"
#include "core/networking/VRNetworkManager.h"
#include "core/utils/VRFunction.h"

#include <boost/bind.hpp>

using namespace OSG;

VRScenegraphInterface::VRScenegraphInterface(string name) : VRObject(name) {
    resetWebsocket();
}

VRScenegraphInterface::~VRScenegraphInterface() {}

VRScenegraphInterfacePtr VRScenegraphInterface::ptr() { return static_pointer_cast<VRScenegraphInterface>( shared_from_this() ); }
VRScenegraphInterfacePtr VRScenegraphInterface::create(string name)  { return VRScenegraphInterfacePtr( new VRScenegraphInterface(name) ); }


void VRScenegraphInterface::clear() { clearChildren(); }

void VRScenegraphInterface::setPort(int p) { if (p == port) return; port = p; resetWebsocket(); }

void VRScenegraphInterface::resetWebsocket() {
    if (socket) VRSceneManager::get()->remSocket(socket->getName());
    socket = VRSceneManager::get()->getSocket(port);
    cb = new VRHTTP_cb( "scenegraph interface callback", boost::bind(&VRScenegraphInterface::ws_callback, this, _1) );
    socket->setHTTPCallback(cb);
    socket->setType("http receive");
}

void VRScenegraphInterface::ws_callback(void* _args) {
	HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) return;

    int ID = args->ws_id;
    string msg = args->ws_data;
    if (args->ws_data.size() == 0) return;
}
