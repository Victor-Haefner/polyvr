#include "VRNetworkClient.h"

using namespace OSG;

VRNetworkClient::VRNetworkClient(string n) : name(n) {}
VRNetworkClient::~VRNetworkClient() {}

string VRNetworkClient::getName() { return name; }
string VRNetworkClient::getProtocol() { return protocol; }
string VRNetworkClient::getConnectedUri() { return uri; }
