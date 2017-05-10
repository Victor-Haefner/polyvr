#include "VRRoadNetwork.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "core/tools/VRPathtool.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"

using namespace OSG;

VRRoadNetwork::VRRoadNetwork() : VRObject("RoadNetwork") {
    tool = VRPathtool::create();
}

VRRoadNetwork::~VRRoadNetwork() {}

VRRoadNetworkPtr VRRoadNetwork::create() { return VRRoadNetworkPtr( new VRRoadNetwork() ); }

void VRRoadNetwork::setOntology(VROntologyPtr o) { ontology = o; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

void VRRoadNetwork::updateTexture() { // TODO: port from python code
    ;
}

VREntityPtr VRRoadNetwork::addNode( Vec3f pos ) {
	auto node = ontology->addEntity("node", "Node");
	int nID = tool->addNode( pose::create(pos, Vec3f(0,0,-1), Vec3f(0,1,0) ) );
	auto handle = tool->getHandle(nID);
	handle->setEntity(node);

    vector<string> posStr;
    posStr.push_back( toString(pos[0]) );
    posStr.push_back( toString(pos[1]) );
    posStr.push_back( toString(pos[2]) );

	node->setVector("position", posStr, "Position");
	node->set("graphID", toString(nID) );
	return node;
}

VREntityPtr VRRoadNetwork::addLane( int direction, VREntityPtr road, float width ) {
	auto l = ontology->addEntity( road->getName()+"Lane", "Lane");
	l->set("width", toString(width));
	l->set("direction", toString(direction));
	road->add("lanes", l->getName());
	return l;
}

VREntityPtr VRRoadNetwork::addRoad( string name, vector<VREntityPtr> paths, int rID, string type ) {
	auto r = ontology->addEntity( name+"Road", type );
	r->set("ID", toString(rID));
	for (auto path : paths) r->add("path", path->getName());
	return r;
}

