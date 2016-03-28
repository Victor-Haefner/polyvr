#include "VRNetwork.h"

using namespace OSG;

VRNetwork::VRNetwork() {}
VRNetwork::~VRNetwork() {}

VRNetwork::NodePtr VRNetwork::addNetworkNode() {
    static int ID = 0; ID++;
    auto n = NodePtr( new Node() );
    nodes[ID] = n;
    n->ID = ID;
    return n;
}

vector<VRNetwork::NodePtr> VRNetwork::getNetworkNodes() {
    vector<NodePtr> res;
    for (auto n : nodes) res.push_back(n.second);
    return res;
}
