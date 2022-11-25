#include "VRNetworkServer.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRNetworkServer::VRNetworkServer(string name) : name(name) {}
VRNetworkServer::~VRNetworkServer() {
    VRSceneManager::get()->subNetworkServer(this);
}

void VRNetworkServer::regServer(VRNetworkServerPtr s) {
    VRSceneManager::get()->regNetworkServer(s);
}

string VRNetworkServer::getName() { return name; }
string VRNetworkServer::getProtocol() { return protocol; }

VRNetworkFlow& VRNetworkServer::getInFlow() { return inFlow; }
VRNetworkFlow& VRNetworkServer::getOutFlow() { return outFlow; }
