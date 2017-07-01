#include "VRRoadIntersection.h"
#include "VRRoad.h"
#include "../VRWorldGenerator.h"
#include "core/utils/toString.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"

using namespace OSG;

VRRoadIntersection::VRRoadIntersection() : VRRoadBase("RoadIntersection") {}
VRRoadIntersection::~VRRoadIntersection() {}

VRRoadIntersectionPtr VRRoadIntersection::create() { return VRRoadIntersectionPtr( new VRRoadIntersection() ); }

void VRRoadIntersection::computeLanes() {
	vector<VREntityPtr> roads = entity->getAllEntities("roads");
	VREntityPtr node = entity->getEntity("node");
	if (!node) { cout << "Warning in VRRoadNetwork::computeIntersectionLanes, intersection node is NULL!" << endl; return; }
	string iN = entity->getName();
	string nN = node->getName();

	// get in and out lanes
	map< VREntityPtr, vector<VREntityPtr> > inLanes; // road is key, vector contains lanes
	map< VREntityPtr, vector<VREntityPtr> > outLanes;
	map< VREntityPtr, int > roadEntrySigns;
	for (VREntityPtr road : roads) {
        VRRoad r;
        r.setEntity(road);
		VREntityPtr roadEntry = r.getNodeEntry(node);
		int reSign = toInt( roadEntry->get("sign")->value );
		roadEntrySigns[road] = reSign;
		for (VREntityPtr lane : road->getAllEntities("lanes")) {
            if (!lane->is_a("Lane")) continue;
			int direction = toInt( lane->get("direction")->value );
			if (direction*reSign == 1) inLanes[road].push_back(lane);
			if (direction*reSign == -1) outLanes[road].push_back(lane);
		}
	}

	auto checkMatchingLanes = [&](int i, int j, int Nin, int Nout, int reSignIn, int reSignOut) { // TODO: extend the cases!
        if (Nin == Nout && i != j && reSignIn != reSignOut) return false;
        if (Nin == Nout && i != Nout-j-1 && reSignIn == reSignOut) return false;
        if (Nin == 1 || Nout == 1) return true;
        return true;
	};

	// compute lane paths
	for (auto roadOut : outLanes) {
        for (auto roadIn : inLanes) {
            if (roadIn.first == roadOut.first) continue;
            int Nin = roadIn.second.size();
            int Nout = roadOut.second.size();
            int reSignIn = roadEntrySigns[roadIn.first];
            int reSignOut = roadEntrySigns[roadOut.first];
            for (int i=0; i<Nin; i++) {
                auto laneIn = roadIn.second[i];
                float width = toFloat( laneIn->get("width")->value );
                auto nodes1 = laneIn->getEntity("path")->getAllEntities("nodes");
                VREntityPtr node1 = *nodes1.rbegin();
                for (int j=0; j<Nout; j++) {
                    if (!checkMatchingLanes(i,j,Nin, Nout, reSignIn, reSignOut)) continue;
                    auto laneOut = roadOut.second[j];
                    auto node2 = laneOut->getEntity("path")->getAllEntities("nodes")[0];
                    auto lane = addLane(1, width);
                    auto nodes = { node1->getEntity("node"), node2->getEntity("node") };
                    auto norms = { node1->getVec3f("direction"), node2->getVec3f("direction") };
                    auto lPath = addPath("Path", "lane", nodes, norms);
                    lane->add("path", lPath->getName());
                }
            }
        }
	}
}

VRGeometryPtr VRRoadIntersection::createGeometry() {
    VRPolygon poly; // intersection VRPolygon
    auto roads = entity->getAllEntities("roads");
    VREntityPtr node = entity->getEntity("node");
    if (!node) return 0;
    for (auto roadEnt : roads) {
        VRRoad road; road.setEntity(roadEnt);
        auto rNode = getRoadNode(roadEnt);
        if (!rNode) continue;
        auto& endP = road.getEdgePoints( rNode );
        poly.addPoint(Vec2f(endP.p1[0], endP.p1[2]));
        poly.addPoint(Vec2f(endP.p2[0], endP.p2[2]));
    }
    poly = poly.getConvexHull();
    Triangulator tri;
    tri.add( poly );
    VRGeometryPtr intersection = tri.compute();
    intersection->setPose(Vec3f(0,0,0), Vec3f(0,1,0), Vec3f(0,0,1));
    intersection->applyTransformation();
	setupTexCoords( intersection, entity );
	return intersection;
}

VREntityPtr VRRoadIntersection::getRoadNode(VREntityPtr roadEnt) {
    /*string iN = intersectionEnt->getName();
    string rN = roadEnt->getName();
    auto res = ontology->process("q(n):Node(n);Road("+rN+");RoadIntersection("+iN+");has("+rN+".path.nodes,n);has("+iN+".path.nodes,n)");
    if (res.size() == 0) { cout << "Warning in createIntersectionGeometry, road " << roadEnt->getName() << " has no nodes!" << endl; return 0; }
    return res[0];*/

    for (auto rp : roadEnt->getAllEntities("path")) {
        for (auto rnE : rp->getAllEntities("nodes")) {
            for (auto ip : entity->getAllEntities("path")) {
                for (auto inE : ip->getAllEntities("nodes")) {
                    auto rn = rnE->getEntity("node");
                    auto in = inE->getEntity("node");
                    if (in == rn) return in;
                }
            }
        }
    }
    return 0;
}

VREntityPtr VRRoadIntersection::addTrafficLight( posePtr p, string asset ) {
    if (auto geo = world->getAsset(asset)) {
        addChild(geo);
        geo->setPose(p);
    }
    return 0;
}

void VRRoadIntersection::computeTrafficLights() {
    for (auto lane : entity->getAllEntities("lanes")) {
        for (auto pathEnt : lane->getAllEntities("path")) {
            auto entry = pathEnt->getEntity("nodes");
            float width = toFloat( lane->get("width")->value );

            Vec3f p = entry->getEntity("node")->getVec3f("position");
            Vec3f n = entry->getVec3f("direction");
            Vec3f x = n.cross(Vec3f(0,1,0));
            x.normalize();
            auto P = pose::create( p + Vec3f(0,3,0), Vec3f(0,-1,0), n);
            addTrafficLight(P, "trafficLight");
        }
    }
}

void VRRoadIntersection::computeMarkings() {
    string name = entity->getName();

    map<VREntityPtr, float> laneEntries;
    for (auto lane : entity->getAllEntities("lanes")) {
        for (auto pathEnt : lane->getAllEntities("path")) {
            auto entry = pathEnt->getEntity("nodes");
            laneEntries[entry] = toFloat( lane->get("width")->value );
        }
    }

    auto addStopLine = [&]( Vec3f p1, Vec3f p2, Vec3f n1, Vec3f n2, float w, int dashNumber) {
		auto node1 = addNode( p1 );
		auto node2 = addNode( p2 );
		auto m = addPath("StopLine", name, {node1, node2}, {n1,n2});
		m->set("width", toString(w)); //  width in meter
		if (dashNumber == 0) m->set("style", "solid"); // simple line
		m->set("style", "dashed"); // dotted line
		m->set("dashNumber", toString(dashNumber)); // dotted line
		entity->add("markings", m->getName());
		return m;
    };

    for (auto e : laneEntries) { // entry/width
        Vec3f p = e.first->getEntity("node")->getVec3f("position");
        Vec3f n = e.first->getVec3f("direction");
        Vec3f x = n.cross(Vec3f(0,1,0));
        x.normalize();
        float W = e.second*0.35;
        float D = 0.4;
        addStopLine( p-x*W+n*D*0.5, p+x*W+n*D*0.5, x, x, D, 0);
    }
}


