#include "VRNetworkClient.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;


VRNetworkFlow::VRNetworkFlow() {
    kbs_queue = vector<double>(30, 0);
}

vector<double>& VRNetworkFlow::getKBperSec() { return kbs_queue; }

void VRNetworkFlow::logFlow(double kb) {
    int N = kbs_queue.size();

    kbs_queue[kbs_queue_ptr] = kb;

    kbs = 0;
    for (auto k : kbs_queue) kbs += k;
    kbs /= N;

    kbs_queue_ptr++;
    kbs_queue_ptr = kbs_queue_ptr%N;
}


VRNetworkClient::VRNetworkClient(string n) : name(n) {}

VRNetworkClient::~VRNetworkClient() {
    VRSceneManager::get()->subNetworkClient(this);
}

string VRNetworkClient::getName() { return name; }
string VRNetworkClient::getProtocol() { return protocol; }
string VRNetworkClient::getConnectedUri() { return uri; }

VRNetworkFlow& VRNetworkClient::getInFlow() { return inFlow; }
VRNetworkFlow& VRNetworkClient::getOutFlow() { return outFlow; }
