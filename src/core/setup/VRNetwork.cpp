#include "VRNetwork.h"
#include "core/utils/VRManager.cpp"

using namespace OSG;

//template<> class VRManager<VRNetworkNode>;
//template<> VRNetworkNodePtr VRManager<VRNetworkNode>::add(string name);

VRNetwork::VRNetwork() : VRManager("Network") {}
VRNetwork::~VRNetwork() { cout << "~VRNetwork"; }


VRNetworkNode::VRNetworkNode(string name) {
    setNameSpace("NetworkNode");
    setName(name);
}

VRNetworkNode::~VRNetworkNode() {}

VRNetworkNodePtr VRNetworkNode::create(string name) { return VRNetworkNodePtr( new VRNetworkNode(name) ); }
