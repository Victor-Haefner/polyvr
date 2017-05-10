#include "VRRoadNetwork.h"

using namespace OSG;

VRRoadNetwork::VRRoadNetwork() : VRObject("RoadNetwork") {}
VRRoadNetwork::~VRRoadNetwork() {}

VRRoadNetworkPtr VRRoadNetwork::create() { return VRRoadNetworkPtr( new VRRoadNetwork() ); }

void VRRoadNetwork::setOntology(VROntologyPtr o) { ontology = o; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

void VRRoadNetwork::updateTexture() { // TODO: port from python code
    ;
}
