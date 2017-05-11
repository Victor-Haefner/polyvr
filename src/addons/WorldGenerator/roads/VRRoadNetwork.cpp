#include "VRRoadNetwork.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/tools/VRPathtool.h"
#include "core/math/pose.h"
#include "core/math/path.h"
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

vector<string> toStringVector(Vec3f& v) {
    vector<string> res;
    res.push_back( toString(v[0]) );
    res.push_back( toString(v[1]) );
    res.push_back( toString(v[2]) );
    return res;
}

VREntityPtr VRRoadNetwork::addNode( Vec3f pos ) {
	auto node = ontology->addEntity("node", "Node");
	int nID = tool->addNode( pose::create(pos, Vec3f(0,0,-1), Vec3f(0,1,0) ) );
	auto handle = tool->getHandle(nID);
	handle->setEntity(node);
	node->setVector("position", toStringVector(pos), "Position");
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

VREntityPtr VRRoadNetwork::addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3f> normals ) {
    auto path = ontology->addEntity(name+"Path", type);
	VREntityPtr lastNode;
	Vec3f nL;
	int N = nodes.size();

	for ( int i = 0; i< N; i++) {
        auto node = nodes[i];
        auto norm = normals[i];
		auto nodeEntry = ontology->addEntity(name+"Entry", "NodeEntry");
		nodeEntry->set("path", path->getName());
		nodeEntry->set("node", node->getName());
		nodeEntry->set("sign", "0");
		if (i == 0) nodeEntry->set("sign", "-1");
		if (i == N-1) nodeEntry->set("sign", "1");
		nodeEntry->setVector("direction", toStringVector(norm), "Direction");

		node->add("paths", nodeEntry->getName());
		path->add("nodes", nodeEntry->getName());

		if (lastNode) {
			int nID1 = toInt(lastNode->get("graphID")->value);
			int nID2 = toInt(node->get("graphID")->value);
			tool->connect(nID1, nID2, 1, nL, norm);
		}
		lastNode = node;
		nL = norm;
	}

	return path;
}

void VRRoadNetwork::computeIntersectionLanes( VREntityPtr intersection ) {
	vector<VREntityPtr> roads = intersection->getAllEntities("roads");
	VREntityPtr node = intersection->getEntity("node");
	string iN = intersection->getName();
	string nN = node->getName();

	auto getRoadEntry = [&](VREntityPtr road, VREntityPtr node) {
		string rN = road->getName();
		auto nodeEntry = ontology->process("q(e):NodeEntry(e);Node("+nN+");Road("+rN+");has("+rN+".path,e);has("+nN+",e)");
		return nodeEntry[0];
	};

	// get in and out lanes
	vector< pair<VREntityPtr, VREntityPtr> > inLanes;
	vector< pair<VREntityPtr, VREntityPtr> > outLanes;
	for (VREntityPtr road : roads) {
		VREntityPtr roadEntry = getRoadEntry(road, node);
		int reSign = toInt( roadEntry->get("sign")->value );
		for (VREntityPtr lane : road->getAllEntities("lanes")) {
			int direction = toInt( lane->get("direction")->value );
			if (direction*reSign == 1) inLanes.push_back(pair<VREntityPtr, VREntityPtr>(lane, road));
			if (direction*reSign == -1) outLanes.push_back(pair<VREntityPtr, VREntityPtr>(lane, road));
		}
	}

	// compute lane paths
	for (auto inRL : inLanes) {
        VREntityPtr laneIn = inRL.first;
        VREntityPtr roadIn = inRL.second;
		float width = toFloat( laneIn->get("width")->value );
		auto nodes1 = laneIn->getEntity("path")->getAllEntities("nodes");
		VREntityPtr node1 = *nodes1.rbegin();
		for (auto outRL : outLanes) {
            VREntityPtr laneOut = outRL.first;
            VREntityPtr roadOut = outRL.second;
			if (roadIn == roadOut) continue;

			auto node2 = laneOut->getEntity("path")->getAllEntities("nodes")[0];
			auto lane = addLane(1, intersection, width);

			vector<VREntityPtr> nodes;
			nodes.push_back( node1->getEntity("node") );
			nodes.push_back( node2->getEntity("node") );

			vector<Vec3f> norms;
			norms.push_back( node1->getVec3f("direction") );
			norms.push_back( node2->getVec3f("direction") );

			auto lPath = addPath("Path", "lane", nodes, norms);
			lane->add("path", lPath->getName());
		}
	}
}

pathPtr VRRoadNetwork::toPath( VREntityPtr pathEntity, int resolution ) {
	vector<Vec3f> pos;
	vector<Vec3f> norms;
	for (auto nodeEntry : pathEntity->getAllEntities("nodes")) {
		pos.push_back( nodeEntry->getEntity("node")->getVec3f("position") );
		norms.push_back(  nodeEntry->getVec3f("direction") );
	}

	pathPtr Path = path::create();
	for (int i=0; i<pos.size(); i++) Path->addPoint(pose(pos[i], norms[i]));
	Path->compute(resolution);
	return Path;
}

void VRRoadNetwork::computeLanePaths( VREntityPtr road ) {
	auto lanes = road->getAllEntities("lanes");
	int Nlanes = lanes.size();

	float roadWidth = 0;
	for (auto lane : lanes) roadWidth += toFloat( lane->get("width")->value );

	for (int li=0; li<Nlanes; li++) {
        auto lane = lanes[li];
		lane->clear("path");
		float width = toFloat( lane->get("width")->value );
		int direction = toInt( lane->get("direction")->value );
		for (auto pathEnt : road->getAllEntities("path")) {
			pathPtr rPath = toPath(pathEnt, 8);
			vector<VREntityPtr> nodes;
			vector<Vec3f> norms;
			for (auto point : rPath->getPoints()) {
				Vec3f p = point.pos();
				Vec3f n = point.dir();
				Vec3f x = Vec3f(0,1,0).cross(n);
				x.normalize();
				float k = width*(0.5+li) - roadWidth*0.5;
				Vec3f pi = x*k + p;
				VREntityPtr node = addNode(pi);
				nodes.push_back(node);
				norms.push_back(n*direction);
			}

			if (direction < 0) {
				reverse(nodes.begin(), nodes.end());
				reverse(norms.begin(), norms.end());
			}
			auto lPath = addPath("Path", "lane", nodes, norms);
			lane->add("path", lPath->getName());
		}
	}
}


