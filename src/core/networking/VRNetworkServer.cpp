#include "VRNetworkServer.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRNetworkServer::VRNetworkServer() {}
VRNetworkServer::~VRNetworkServer() {
    VRSceneManager::get()->subNetworkServer(this);
}

VRNetworkServerPtr VRNetworkServer::create() { return VRNetworkServerPtr( new VRNetworkServer() ); }
VRNetworkServerPtr VRNetworkServer::ptr() { return static_pointer_cast<VRNetworkServer>(shared_from_this()); }

string VRNetworkServer::getName() { return name; }
string VRNetworkServer::getProtocol() { return protocol; }

VRNetworkFlow& VRNetworkServer::getInFlow() { return inFlow; }
VRNetworkFlow& VRNetworkServer::getOutFlow() { return outFlow; }
