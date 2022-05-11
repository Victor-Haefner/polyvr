#include "VRNetworkClient.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRNetworkClient::VRNetworkClient(string n) : name(n) {}

VRNetworkClient::~VRNetworkClient() {
    VRSceneManager::get()->subNetworkClient(this);
}

string VRNetworkClient::getName() { return name; }
string VRNetworkClient::getProtocol() { return protocol; }
string VRNetworkClient::getConnectedUri() { return uri; }
