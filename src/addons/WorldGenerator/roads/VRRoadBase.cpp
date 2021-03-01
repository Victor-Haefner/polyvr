#include "VRRoadBase.h"
#include "VRRoadNetwork.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/math/path.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include <OpenSG/OSGGeoProperties.h>

const double pi = 3.14159265359;

using namespace OSG;


VRRoadBase::VRRoadBase(string name) : VRObject(name) {}
VRRoadBase::~VRRoadBase() {}

vector<string> VRRoadBase::toStringVector(const Vec3d& v) {
    vector<string> res;
    res.push_back( toString(v[0]) );
    res.push_back( toString(v[1]) );
    res.push_back( toString(v[2]) );
    return res;
}

PathPtr VRRoadBase::toPath( VREntityPtr pathEntity, int resolution ) {
    if (!pathEntity) return 0;
	vector<Vec3d> pos;
	vector<Vec3d> norms;
	auto nodes = pathEntity->getAllEntities("nodes");
	for (auto& nodeEntry : nodes) {
        auto node = nodeEntry->getEntity("node");
        if (!node) { cout << "Warning in VRRoadBase::toPath, path node is NULL!" << endl; continue; }
		pos.push_back( node->getVec3("position") );
		norms.push_back(  nodeEntry->getVec3("direction") );
	}

	PathPtr Path = Path::create();
	for (unsigned int i=0; i<pos.size(); i++) Path->addPoint(Pose(pos[i], norms[i]));
	Path->compute(resolution);
	return Path;
}

void VRRoadBase::setupTexCoords( VRGeometryPtr geo, VREntityPtr way ) {
	int rID = toInt( way->get("ID")->value );
	GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
	for (int i=0; i<geo->size(); i++) tcs->addValue(Vec2d(rID, 0));
	geo->setPositionalTexCoords2D(1.0, 0, Vec2i(0,2)); // positional tex coords
	geo->setTexCoords(tcs, 1); // add another tex coords set
}

VREntityPtr VRRoadBase::addNode( int nodeID, Vec3d pos, bool elevate, float elevationOffset ) {
    auto o = ontology.lock();
    auto t = terrain.lock();
    if (!o) return 0; // TODO
    //cout << "VRRoadBase::addNode " << pos;
    auto roads = world.lock()->getRoadNetwork();
    if (t && elevate) pos = t->elevatePoint(pos, roads->getTerrainOffset() + elevationOffset);
    //cout << " -> " << pos << endl;

    auto node = o->addEntity("node", "Node");
    //return 0;
	node->setVector("position", toStringVector(pos), "Position");
    node->set("graphID", toString(nodeID) );

	/*if (tool) {
        int nID = tool->addNode( pose::create(pos, Vec3d(0,0,-1), Vec3d(0,1,0) ) );
        auto handle = tool->getHandle(nID);
        handle->setEntity(node);
        node->set("graphID", toString(nID) );
	}*/
    return node;
}

VREntityPtr VRRoadBase::addLane( int direction, float width, bool pedestrian ) {
    auto o = ontology.lock();
	auto l = o->addEntity( entity->getName()+"Lane", "Lane");
	l->set("width", toString(width));
	l->set("direction", toString(direction));
	l->set("pedestrian", toString(pedestrian));
	entity->add("lanes", l->getName());
	l->set("road", entity->getName());
	//for (int eID : edgeIDs) l->add("graphIDs", eID);
	return l;
}

VREntityPtr VRRoadBase::addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3d> normals ) {
    auto o = ontology.lock();
    auto path = o->addEntity(name+"Path", type);
	//VREntityPtr lastNode;
	Vec3d nL;
	int N = nodes.size();

	for ( int i = 0; i< N; i++) {
        auto node = nodes[i];
        if (!node) { cout << "Warning in VRRoadBase::addPath, NULL node!" << endl; continue; }
        auto norm = normals[i];
		auto nodeEntry = o->addEntity(name+"Entry", "NodeEntry");
		nodeEntry->set("path", path->getName());
		nodeEntry->set("node", node->getName());
		nodeEntry->set("sign", "0");
		if (i == 0) nodeEntry->set("sign", "-1");
		if (i == N-1) nodeEntry->set("sign", "1");
		nodeEntry->setVector("direction", toStringVector(norm), "Direction");

		node->add("paths", nodeEntry->getName());
		path->add("nodes", nodeEntry->getName());

		/*if (lastNode && tool) {
			int nID1 = toInt(lastNode->get("graphID")->value);
			int nID2 = toInt(node->get("graphID")->value);
			tool->connect(nID1, nID2, 1, nL, norm);
		}
		lastNode = node;*/
		nL = norm;
	}

	return path;
}

VREntityPtr VRRoadBase::addArrows( VREntityPtr lane, float t, vector<float> dirs, int type ) {
    auto o = ontology.lock();
    auto arrow = o->addEntity("laneArrow", "Arrow");
    lane->add("arrows", arrow->getName());
    arrow->set("position", toString(t));
    arrow->set("lane", lane->getName());
    arrow->set("type", toString(type));
    for (auto d : dirs) arrow->add("direction", toString(d));
    return arrow;
}

VRGeometryPtr VRRoadBase::addPole( Vec3d P1, Vec3d P4, float radius ) {
    auto w = world.lock();
    auto p = Path::create();
    Color3f gray(0.4,0.4,0.4);
    Vec3d Y(0,1,0);
    Vec3d pN = Vec3d(0,0,1); // normal of the plane where the pole lies in
    Vec3d P2 = P1; P2[1] = P4[1] - 0.5;
    Vec3d P3 = P1; P3[1] = P4[1];
    Vec3d D = P4-P1; D[1] = 0;
    bool curved = (D.squareLength() > 1e-3);

    if (curved) {
        pN = D.cross(Y);
        pN.normalize();
        D.normalize();
        P3 += D*0.5;
    }

    p->addPoint(Pose(P1, Y, pN), gray);
    if (curved) {
        p->addPoint(Pose(P2, Y*2, pN), gray);
        p->addPoint(Pose(P3, D*2, pN), gray);
        p->addPoint(Pose(P4, D, pN), gray);
        p->compute(8);
    } else {
        p->addPoint(Pose(P4, Y, pN), gray);
        p->compute(2);
    }

    int N = 8;
    vector<Vec3d> profile;
    for (int i=0; i<N; i++) {
        float a = i*(2*pi/N);
        profile.push_back(Vec3d(cos(a), sin(a), 0)*radius);
    }

    auto s = VRStroke::create("pole");
    s->setPath(p);
    s->strokeProfile(profile, true, true);
    s->setMaterial( w->getMaterial("phong") );
    return s;
}
