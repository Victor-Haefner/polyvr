#include "VRRoadIntersection.h"
#include "VRRoad.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/utils/toString.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRObjectManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"

using namespace OSG;

VRRoadIntersection::VRRoadIntersection() : VRRoadBase("RoadIntersection") {}
VRRoadIntersection::~VRRoadIntersection() {}

VRRoadIntersectionPtr VRRoadIntersection::create() { return VRRoadIntersectionPtr( new VRRoadIntersection() ); }

void VRRoadIntersection::computeLanes() {
	VREntityPtr node = entity->getEntity("node");
	if (!node) { cout << "Warning in VRRoadNetwork::computeIntersectionLanes, intersection node is NULL!" << endl; return; }
	string iN = entity->getName();
	string nN = node->getName();

	// get in and out lanes
	inLanes.clear();
	outLanes.clear();
	map< VRRoadPtr, int > roadEntrySigns;
	for (auto road : roads) {
		VREntityPtr roadEntry = road->getNodeEntry(node);
		int reSign = toInt( roadEntry->get("sign")->value );
		roadEntrySigns[road] = reSign;
		for (VREntityPtr lane : road->getEntity()->getAllEntities("lanes")) {
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
                float width = laneIn->getValue<float>("width");
                bool pedestrianIn = laneIn->getValue<bool>("pedestrian");
                auto nodes1 = laneIn->getEntity("path")->getAllEntities("nodes");
                VREntityPtr node1 = *nodes1.rbegin();
                for (int j=0; j<Nout; j++) {
                    if (!checkMatchingLanes(i,j,Nin, Nout, reSignIn, reSignOut)) continue;
                    auto laneOut = roadOut.second[j];
                    bool pedestrianOut = laneOut->getValue<bool>("pedestrian");
                    auto node2 = laneOut->getEntity("path")->getAllEntities("nodes")[0];
                    auto lane = addLane(1, width, pedestrianIn || pedestrianOut);
                    auto nodes = { node1->getEntity("node"), node2->getEntity("node") };
                    auto norms = { node1->getVec3f("direction"), node2->getVec3f("direction") };
                    auto lPath = addPath("Path", "lane", nodes, norms);
                    lane->add("path", lPath->getName());
                    nextLanes[laneIn].push_back(lane);
                    nextLanes[lane].push_back(laneOut);
                }
            }
        }
	}
}

VRGeometryPtr VRRoadIntersection::createGeometry() {
    VRPolygon poly;
    VREntityPtr node = entity->getEntity("node");
    if (!node) return 0;
    for (auto road : roads) {
        auto rNode = getRoadNode(road->getEntity());
        if (!rNode) continue;
        auto& endP = road->getEdgePoints( rNode );
        poly.addPoint(Vec2d(endP.p1[0], endP.p1[2]));
        poly.addPoint(Vec2d(endP.p2[0], endP.p2[2]));
    }
    for (auto p : intersectionPoints) poly.addPoint(Vec2d(p[0], p[2]));
    poly = poly.getConvexHull();
    if (poly.size() <= 2) return 0;

    median = poly.getBoundingBox().center();
    poly.translate(-median);
    Triangulator tri;
    tri.add( poly );
    VRGeometryPtr intersection = tri.compute();
    if (intersection->size() == 0) { cout << "VRRoadIntersection::createGeometry ERROR: no geometry created!\n"; return 0; }
    if (terrain) terrain->elevatePoint(median, roadTerrainOffset); // TODO: elevate each point of the polygon
    intersection->setPose(median, Vec3d(0,1,0), Vec3d(0,0,1));
    intersection->applyTransformation();
	setupTexCoords( intersection, entity );
	perimeter = poly.shrink(markingsWidth*0.5);
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

void VRRoadIntersection::addRoad(VRRoadPtr road) {
    roads.push_back(road);
    entity->add("roads", road->getEntity()->getName());
}

VREntityPtr VRRoadIntersection::addTrafficLight( posePtr p, string asset, Vec3d root) {
    float R = 0.05;
    if (auto geo = world->getAssetManager()->copy(asset, p)) {
        addChild(geo);
        geo->move(R);
    }
    VRGeometryPtr pole = addPole(root, p->pos(), R);
    addChild(pole);
    return 0;
}

void VRRoadIntersection::computeTrafficLights() { // deprecated
    /*
    if (roads.size() == 2) return;
    if (inLanes.size() == 1) return;

    auto node = entity->getEntity("node");
    for (auto road : inLanes) {
        auto eP = road.first->getEdgePoints( node );
        Vec3d root = eP.p1;
        for (auto lane : road.second) {
            for (auto pathEnt : lane->getAllEntities("path")) {
                auto entries = pathEnt->getAllEntities("nodes");
                auto entry = entries[entries.size()-1];
                float width = toFloat( lane->get("width")->value );

                Vec3d p = entry->getEntity("node")->getVec3f("position");
                Vec3d n = entry->getVec3f("direction");
                Vec3d x = n.cross(Vec3d(0,1,0));
                x.normalize();
                auto P = pose::create( p + Vec3d(0,3,0), Vec3d(0,-1,0), n);
                addTrafficLight(P, "trafficLight", root);
            }
        }
    }
    */
}

void VRRoadIntersection::computeMarkings() {
    if (!perimeter) return;
    string name = entity->getName();

    auto addLine = [&]( const string& type, Vec3d p1, Vec3d p2, Vec3d n1, Vec3d n2, float w, int dashNumber) {
		auto node1 = addNode( p1 );
		auto node2 = addNode( p2 );
		auto m = addPath(type, name, {node1, node2}, {n1,n2});
		m->set("width", toString(w)); //  width in meter
		if (dashNumber == 0) m->set("style", "solid"); // simple line
		m->set("style", "dashed"); // dotted line
		m->set("dashNumber", toString(dashNumber)); // dotted line
		entity->add("markings", m->getName());
		return m;
    };

    bool isPedestrian = false;
    for (auto road : inLanes) for (auto lane : road.second) { if (lane->getValue<bool>("pedestrian")) isPedestrian = true; break; }

    int inCarLanes = 0;
    for (auto road : inLanes) {
        bool pedestrian = false;
        for (auto lane : road.second)
            for (auto l : nextLanes[lane]) if (l->getValue<bool>("pedestrian")) pedestrian = true;
        inCarLanes += pedestrian?0:1;
    }

    for (auto road : inLanes) {
        for (auto lane : road.second) {
            for (auto pathEnt : lane->getAllEntities("path")) {
                auto entry = pathEnt->getEntity("nodes",-1);
                auto node = entry->getEntity("node");
                if (lane->getValue<bool>("pedestrian")) continue;

                float W = lane->getValue<float>("width");
                Vec3d p = node->getVec3f("position");
                Vec3d n = entry->getVec3f("direction");
                Vec3d x = n.cross(Vec3d(0,1,0));
                n.normalize();
                x.normalize();

                if (inCarLanes >= 2) { // stop lines
                    float D = 0.4;
                    float w = 0.35;
                    addLine( "StopLine", p-x*W*w+n*D*0.5, p+x*W*w+n*D*0.5, x, x, D, 0);
                }

                // arrows
                vector<float> directions;
                for (auto e : node->getAllEntities("paths")){
                    if (e == entry) continue;
                    auto n2 = e->getEntity("path")->getEntity("nodes",-1)->getVec3f("direction");
                    n2.normalize();
                    Vec3d w = n.cross(n2);
                    float a = asin( w.length() );
                    if (w[1] < 0) a = -a;
                    directions.push_back(a);
                }
                addArrows( lane, -5, directions );
            }
        }
    }

    if (!isPedestrian) {
         // border markings
        vector<Vec3d> points;
        for (int i=0; i<perimeter->size(); i++) {
            auto p = perimeter->getPoint(i);
            points.push_back( Vec3d(p[0], 0, p[1]) + median );
        }

        auto isRoadEdge = [&](const Vec3d& p1, const Vec3d& p2) {
            auto pm = (p1+p2)*0.5;
            for (auto rf : roadFronts) {
                float L = (pm-rf.first.pos()).squareLength();
                if (L < rf.second*rf.second*0.1) return true; // ignore road edges
                //cout << pm << "  " << rf.first.pos() << "  " << sqrt(L) << "  " << rf.second << endl;
            }
            return false;
        };

        for (int i=0; i<points.size(); i++) {
            auto p1 = points[i];
            auto p2 = points[(i+1)%points.size()];
            if (isRoadEdge(p1, p2)) continue;
            Vec3d n = p2-p1; n.normalize();
            p1 -= n*markingsWidth*0.5;
            p2 += n*markingsWidth*0.5;
            addLine( "RoadMarking", p1, p2, n, n, markingsWidth, 0);
        }
    }
}

void VRRoadIntersection::computeLayout() {
    auto node = entity->getEntity("node");
    Vec3d pNode = node->getVec3f("position");
    int N = roads.size();

    // sort roads
    auto compare = [&](VRRoadPtr road1, VRRoadPtr road2) -> bool {
        Vec3d norm1 = road1->getEdgePoints( node ).n;
        Vec3d norm2 = road2->getEdgePoints( node ).n;
        float K = norm1.cross(norm2)[1];
        return (K < 0);
    };

    sort( roads.begin(), roads.end(), compare );

    auto intersect = [&](const Pnt3d& p1, const Vec3d& n1, const Pnt3d& p2, const Vec3d& n2) -> Vec3d {
        Vec3d d = p2-p1;
        Vec3d n3 = n1.cross(n2);
        float N3 = n3.dot(n3);
        if (N3 == 0) N3 = 1.0;
        float s = d.cross(n2).dot(n3)/N3;
        return Vec3d(p1) + n1*s;
    };

    auto resolveEdgeIntersections = [&]() {
        for (int r = 0; r<N; r++) { // compute intersection points
            auto road1 = roads[r];
            auto road2 = roads[(r+1)%N];
            auto& data1 = road1->getEdgePoints( node );
            auto& data2 = road2->getEdgePoints( node );
            Vec3d Pi = intersect(data1.p2, data1.n, data2.p1, data2.n);
            data1.p2 = Pi;
            data2.p1 = Pi;
            intersectionPoints.push_back(Pi);
        }
    };

    auto resolveSpacialCases = [&]() {
        if (N == 2) { // special cases for 2 roads
            auto road1 = roads[0];
            auto road2 = roads[1];
            auto& data1 = road1->getEdgePoints( node );
            auto& data2 = road2->getEdgePoints( node );
            cout << " N1 " << data1.n << "  N2 " << data2.n << "  Dot " << data1.n.dot(data2.n) << endl;
            if (data1.n.dot(data2.n) < -0.8) {
                ; // TODO
                return true;
            }
        }
        return false;
    };

    if (!resolveSpacialCases()) resolveEdgeIntersections();

    for (auto road : roads) { // compute road front
        auto& data = road->getEdgePoints( node );
        Vec3d p1 = data.p1;
        Vec3d p2 = data.p2;
        Vec3d norm = data.n;
        float d1 = (p1-pNode).dot(norm);
        float d2 = (p2-pNode).dot(norm);
        float d = min(d1,d2);
        data.p1 = p1-norm*(d1-d);
        data.p2 = p2-norm*(d2-d);

        Vec3d pm = (data.p1 + data.p2)*0.5; // compute road node
        auto n = addNode(pm);
        data.entry->set("node", n->getName());
        n->add("paths", data.entry->getName());
        roadFronts.push_back( make_pair(pose(pm, norm), road->getWidth()) );
    }

    vector<VREntityPtr> iPaths;
    for (uint i=0; i<roads.size(); i++) { // compute intersection paths
        auto road1 = roads[i];
        auto rEntry1 = road1->getNodeEntry(node);
        if (!rEntry1) continue;
        int s1 = toInt(rEntry1->get("sign")->value);
        Vec3d norm1 = rEntry1->getVec3f("direction");
        auto& data1 = road1->getEdgePoints( node );
        VREntityPtr node1 = data1.entry->getEntity("node");
        if (s1 == 1) {
            for (uint j=0; j<roads.size(); j++) { // compute intersection paths
                auto road2 = roads[j];
                if (j == i) continue;
                auto rEntry2 = road2->getNodeEntry(node);
                if (!rEntry2) continue;
                int s2 = toInt(rEntry2->get("sign")->value);
                if (s2 != -1) continue;
                Vec3d norm2 = rEntry2->getVec3f("direction");
                auto& data2 = road2->getEdgePoints( node );
                VREntityPtr node2 = data2.entry->getEntity("node");
                auto pathEnt = addPath("Path", "intersection", {node1, node2}, {norm1, norm2});
                iPaths.push_back(pathEnt);
            }
        }
    }
    for (auto path : iPaths) entity->add("path", path->getName());
}

