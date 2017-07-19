#include "VRRoadBase.h"
#include "../VRWorldGenerator.h"
#include "core/utils/toString.h"
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

vector<string> VRRoadBase::toStringVector(Vec3d& v) {
    vector<string> res;
    res.push_back( toString(v[0]) );
    res.push_back( toString(v[1]) );
    res.push_back( toString(v[2]) );
    return res;
}

pathPtr VRRoadBase::toPath( VREntityPtr pathEntity, int resolution ) {
    if (!pathEntity) return 0;
	vector<Vec3d> pos;
	vector<Vec3d> norms;
	for (auto nodeEntry : pathEntity->getAllEntities("nodes")) {
        auto node = nodeEntry->getEntity("node");
        if (!node) { cout << "Warning in VRRoadBase::toPath, path node is NULL!" << endl; continue; }
		pos.push_back( node->getVec3f("position") );
		norms.push_back(  nodeEntry->getVec3f("direction") );
	}

	pathPtr Path = path::create();
	for (uint i=0; i<pos.size(); i++) Path->addPoint(pose(pos[i], norms[i]));
	Path->compute(resolution);
	return Path;
}

void VRRoadBase::setWorld(VRWorldGeneratorPtr w) { world = w; }

void VRRoadBase::setupTexCoords( VRGeometryPtr geo, VREntityPtr way ) {
	int rID = toInt( way->get("ID")->value );
	GeoVec2fPropertyRecPtr tcs = GeoVec2fProperty::create();
	for (int i=0; i<geo->size(); i++) tcs->addValue(Vec2d(rID, 0));
	geo->setPositionalTexCoords2D(1.0, 0, Vec2i(0,2)); // positional tex coords
	geo->setTexCoords(tcs, 1); // add another tex coords set
}

VREntityPtr VRRoadBase::addNode( Vec3d pos ) {
	auto node = world->getOntology()->addEntity("node", "Node");
	node->setVector("position", toStringVector(pos), "Position");

	/*if (tool) {
        int nID = tool->addNode( pose::create(pos, Vec3d(0,0,-1), Vec3d(0,1,0) ) );
        auto handle = tool->getHandle(nID);
        handle->setEntity(node);
        node->set("graphID", toString(nID) );
	}*/
	return node;
}

VREntityPtr VRRoadBase::addLane( int direction, float width ) {
	auto l = world->getOntology()->addEntity( entity->getName()+"Lane", "Lane");
	l->set("width", toString(width));
	l->set("direction", toString(direction));
	entity->add("lanes", l->getName());
	return l;
}

VREntityPtr VRRoadBase::addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3d> normals ) {
    auto path = world->getOntology()->addEntity(name+"Path", type);
	//VREntityPtr lastNode;
	Vec3d nL;
	int N = nodes.size();

	for ( int i = 0; i< N; i++) {
        auto node = nodes[i];
        if (!node) { cout << "Warning in VRRoadBase::addPath, NULL node!" << endl; continue; }
        auto norm = normals[i];
		auto nodeEntry = world->getOntology()->addEntity(name+"Entry", "NodeEntry");
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

VRGeometryPtr VRRoadBase::addPole( Vec3d P1, Vec3d P4, float radius ) {
    auto p = path::create();
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

    p->addPoint(pose(P1, Y, pN), gray);
    if (curved) {
        p->addPoint(pose(P2, Y*2, pN), gray);
        p->addPoint(pose(P3, D*2, pN), gray);
        p->addPoint(pose(P4, D, pN), gray);
        p->compute(8);
    } else {
        p->addPoint(pose(P4, Y, pN), gray);
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
    s->setMaterial( world->getMaterial("phong") );
    return s;
}
