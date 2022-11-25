#include "VRNetworkClient.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/system/VRSystem.h"

using namespace OSG;


VRNetworkFlow::VRNetworkFlow() {
    Nq = 100;
    Nv = 30;
    kbps = vector<double>(Nv, 0);
    kb_queue = vector<pair<long long, double>>(Nq, make_pair<long long, double>(0,0));
}

vector<double>& VRNetworkFlow::getKBperSec() { updateKBpSec(); return kbps; }

void VRNetworkFlow::logFlow(double kb) {
    if (kb_queue_ptr == 0) updateKBpSec();
    long long now = getTime();
    kb_queue[kb_queue_ptr] = make_pair(now,kb);
    kb_queue_ptr++;
    kb_queue_ptr = kb_queue_ptr%Nq;
}

void VRNetworkFlow::updateKBpSec() {
    kbps = vector<double>(Nv, 0);
    long long now = getTime();

    int i=0;
    for (auto f : kb_queue) {
        long long t = f.first;
        auto dt = now-t; // microsecs
        if (dt > Nv*1e5) continue; // if too old, ignore

        int ki = floor(dt/1e5);
        kbps[ki] += f.second;
    }
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
