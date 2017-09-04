#include "VRRoadIntersection.h"
#include "VRRoadNetwork.h"
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

	inLanes.clear();
	outLanes.clear();
	laneMatches.clear();
	nextLanes.clear();
	map< VRRoadPtr, int > roadEntrySigns;

	auto getInAndOutLanes = [&]() {
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
	};

	auto computeLaneMatches = [&]() {
	    auto checkDefaultMatch = [](int i, int j, int Nin, int Nout, int reSignIn, int reSignOut) {
            if (Nin == Nout && i != j && reSignIn != reSignOut) return false;
            if (Nin == Nout && i != Nout-j-1 && reSignIn == reSignOut) return false;
            if (Nin == 1 || Nout == 1) return true;
            return true;
        };

	    auto checkContinuationMatch = [](int i, int j, int Nin, int Nout) {
	        if (Nout == Nin && i == j) return true;
	        if (Nout > Nin && i == Nout-j-1) return true; // TODO
	        if (Nout < Nin && Nin-i-1 == j) return true; // TODO
            //if (Nin == Nout && i != j && reSignIn != reSignOut) return false;
            //if (Nin == Nout && i != Nout-j-1 && reSignIn == reSignOut) return false;
            //if (Nin == 1 || Nout == 1) return true;
            return false;
        };

        for (auto roadOut : outLanes) {
            for (auto roadIn : inLanes) {
                if (roadIn.first == roadOut.first) continue;
                int Nin = roadIn.second.size();
                int Nout = roadOut.second.size();
                int reSignIn = roadEntrySigns[roadIn.first];
                int reSignOut = roadEntrySigns[roadOut.first];
                for (int i=0; i<Nin; i++) {
                    auto laneIn = roadIn.second[i];
                    for (int j=0; j<Nout; j++) {
                        auto laneOut = roadOut.second[j];
                        bool match = false;
                        switch (type) {
                            case CONTINUATION: match = checkContinuationMatch(i,j,Nin, Nout); break;
                            //case FORK: break;
                            //case MERGE: break;
                            //case UPLINK: break;
                            default: match = checkDefaultMatch(i,j,Nin, Nout, reSignIn, reSignOut); break;
                        }
                        if (match) laneMatches.push_back(make_pair(laneIn, laneOut));
                    }
                }
            }
        }
	};

	auto mergeMatchingLanes = [&]() {
	    map<VREntityPtr, Vec3d> displacements; // map roads to displace!
	    map<VREntityPtr, bool> processedLanes; // keep list of already processed lanes
        for (auto match : laneMatches) {
            auto laneIn = match.first;
            auto laneOut = match.second;
            auto roadIn = laneIn->getEntity("road");
            auto roadOut = laneOut->getEntity("road");
            int Nin = roadIn->getAllEntities("lanes").size();
            int Nout = roadOut->getAllEntities("lanes").size();
            auto nodes1 = laneIn->getEntity("path")->getAllEntities("nodes");
            auto nodes2 = laneOut->getEntity("path")->getAllEntities("nodes");
            VREntityPtr nodeEnt1 = *nodes1.rbegin();
            VREntityPtr nodeEnt2 = nodes2[0];
            Vec3d X = nodeEnt2->getEntity("node")->getVec3("position") - nodeEnt1->getEntity("node")->getVec3("position");
            float D = X.length();

            if (Nin >= Nout) {
                auto node1 = nodes1[nodes1.size()-2]->getEntity("node");
                auto norm1 = nodes1[nodes1.size()-2]->getVec3("direction");
                auto node2 = nodeEnt2->getEntity("node");
                auto norm2 = nodeEnt2->getVec3("direction");
                nodeEnt1->set("node", node2->getName());
                if (D > 0) displacements[roadIn] = X;
                world->getRoadNetwork()->connectGraph({node1,node2}, {norm1,norm2});
            }
            if (Nin < Nout) {
                auto node1 = nodeEnt1->getEntity("node");
                auto norm1 = nodeEnt1->getVec3("direction");
                auto node2 = nodes2[1]->getEntity("node");
                auto norm2 = nodes2[1]->getVec3("direction");
                nodeEnt2->set("node", node1->getName());
                if (D > 0) displacements[roadOut] = -X;
                world->getRoadNetwork()->connectGraph({node1,node2}, {norm1,norm2});
            }

            processedLanes[laneIn] = true;
            processedLanes[laneOut] = true;
        }

        if (displacements.size() == 0) return;

        for (int i=0; i<roads.size(); i++) { // shift whole road fronts!
            auto& road = roads[i];
            auto& rfront = roadFronts[i];
            auto rEnt = road->getEntity();
            if (!displacements.count(rEnt)) continue;

            Vec3d X = displacements[rEnt];
            road->setOffset(X.dot(rfront.first.x())); // TODO: maybe wrong?
            //Vec3d X = rfront.first.x() * D; // displacement vector
            //auto node = getRoadNode( rEnt );
            //auto p = node->getVec3("position");
            if (inLanes.count(road)) {
                for (auto laneIn : inLanes[road]) {
                    if (processedLanes.count(laneIn)) continue;
                    auto nodes = laneIn->getEntity("path")->getAllEntities("nodes");
                    VREntityPtr node = (*nodes.rbegin())->getEntity("node");
                    auto p = node->getVec3("position");
                    p += X;
                    node->setVec3("position", p, "Position");
                }
            }

            if (outLanes.count(road)) {
                for (auto laneOut : outLanes[road]) {
                    if (processedLanes.count(laneOut)) continue;
                    VREntityPtr node = laneOut->getEntity("path")->getAllEntities("nodes")[0]->getEntity("node");
                    auto p = node->getVec3("position");
                    p += X;
                    node->setVec3("position", p, "Position");
                }
            }


            // TODO: get x, orthogonal vector to road front, and shift all lane nodes (not the road node), and add offset to road, used later for offsetting the geometry
            // TODO: update road graph!!!
        }
	};

	auto bridgeMatchingLanes = [&]() {
        for (auto match : laneMatches) {
            auto laneIn = match.first;
            auto laneOut = match.second;

            float width = laneIn->getValue<float>("width", 0.5);
            bool pedestrianIn = laneIn->getValue<bool>("pedestrian", false);
            auto nodes1 = laneIn->getEntity("path")->getAllEntities("nodes");
            VREntityPtr node1 = *nodes1.rbegin();

            bool pedestrianOut = laneOut->getValue<bool>("pedestrian", false);
            auto node2 = laneOut->getEntity("path")->getAllEntities("nodes")[0];
            auto lane = addLane(1, width, pedestrianIn || pedestrianOut);
            auto nodes = { node1->getEntity("node"), node2->getEntity("node") };
            auto norms = { node1->getVec3("direction"), node2->getVec3("direction") };
            auto lPath = addPath("Path", "lane", nodes, norms);
            lane->add("path", lPath->getName());
            nextLanes[laneIn].push_back(lane);
            nextLanes[lane].push_back(laneOut);
            world->getRoadNetwork()->connectGraph(nodes, norms);
        }
	};

    getInAndOutLanes();
    computeLaneMatches();

    switch (type) {
        case CONTINUATION: mergeMatchingLanes(); break;
        //case FORK: break;
        //case MERGE: break;
        //case UPLINK: break;
        default: bridgeMatchingLanes(); break;
    }
}

void VRRoadIntersection::computePatch() {
    VREntityPtr node = entity->getEntity("node");
    if (!node) return;
    patch = VRPolygon::create();
    for (auto road : roads) {
        auto rNode = getRoadNode(road->getEntity());
        if (!rNode) continue;
        auto& endP = road->getEdgePoints( rNode );
        patch->addPoint(Vec2d(endP.p1[0], endP.p1[2]));
        patch->addPoint(Vec2d(endP.p2[0], endP.p2[2]));
    }
    for (auto p : intersectionPoints) patch->addPoint(Vec2d(p[0], p[2]));
    *patch = patch->getConvexHull();
    if (patch->size() <= 2) { patch.reset(); return; }

    median = patch->getBoundingBox().center();
    patch->translate(-median);
	perimeter = patch->shrink(markingsWidth*0.5);
    patch->translate(median);
    if (terrain) terrain->elevatePolygon(patch, roadTerrainOffset);
    patch->translate(-median);
    if (terrain) terrain->elevatePoint(median, roadTerrainOffset); // TODO: elevate each point of the polygon
}

VRGeometryPtr VRRoadIntersection::createGeometry() {
    if (!patch) return 0;
    if (type != DEFAULT) return 0;
    Triangulator tri;
    tri.add( *patch );
    VRGeometryPtr intersection = tri.compute();
    if (intersection->size() == 0) { cout << "VRRoadIntersection::createGeometry ERROR: no geometry created!\n"; return 0; }
    auto p = median; p[1] = 0;
    intersection->setPose(p, Vec3d(0,0,-1), Vec3d(0,1,0));
    intersection->applyTransformation();
	setupTexCoords( intersection, entity );
	addChild(intersection);
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
		auto node1 = addNode( 0, p1 );
		auto node2 = addNode( 0, p2 );
		auto m = addPath(type, name, {node1, node2}, {n1,n2});
		m->set("width", toString(w)); //  width in meter
		if (dashNumber == 0) m->set("style", "solid"); // simple line
		m->set("style", "dashed"); // dotted line
		m->set("dashNumber", toString(dashNumber)); // dotted line
		entity->add("markings", m->getName());
		return m;
    };

    bool hasMarkings = true;
    for (auto road : roads) if (!road->hasMarkings()) return;

    bool isPedestrian = false;
    for (auto road : inLanes) for (auto lane : road.second) { if (lane->getValue<bool>("pedestrian", false)) isPedestrian = true; break; }

    int inCarLanes = 0;
    for (auto road : inLanes) {
        bool pedestrian = false;
        for (auto lane : road.second)
            for (auto l : nextLanes[lane]) if (l->getValue<bool>("pedestrian", false)) pedestrian = true;
        inCarLanes += pedestrian?0:1;
    }

    for (auto road : inLanes) {
        for (auto lane : road.second) {
            for (auto pathEnt : lane->getAllEntities("path")) {
                auto entry = pathEnt->getEntity("nodes",-1);
                auto node = entry->getEntity("node");
                if (lane->getValue<bool>("pedestrian", false)) continue;

                float W = lane->getValue<float>("width", 0.5);
                Vec3d p = node->getVec3("position");
                Vec3d n = entry->getVec3("direction");
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
                    auto n2 = e->getEntity("path")->getEntity("nodes",-1)->getVec3("direction");
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

void VRRoadIntersection::computeLayout(GraphPtr graph) {
    auto node = entity->getEntity("node");
    Vec3d pNode = node->getVec3("position");
    int N = roads.size();

    // sort roads
    auto compare = [&](VRRoadPtr road1, VRRoadPtr road2) -> bool {
        Vec3d norm1 = road1->getEdgePoints( node ).n;
        Vec3d norm2 = road2->getEdgePoints( node ).n;
        float K = norm1.cross(norm2)[1];
        return (K < 0);
    };

    auto intersect = [&](const Pnt3d& p1, const Vec3d& n1, const Pnt3d& p2, const Vec3d& n2) -> Vec3d {
        Vec3d d = p2-p1;
        Vec3d n3 = n1.cross(n2);
        float N3 = n3.dot(n3);
        if (N3 == 0) N3 = 1.0;
        float s = d.cross(n2).dot(n3)/N3;
        return Vec3d(p1) + n1*s;
    };

    auto getRoadConnectionAngle = [&](VRRoadPtr road1, VRRoadPtr road2) {
        auto& data1 = road1->getEdgePoints( node );
        auto& data2 = road2->getEdgePoints( node );
        return data1.n.dot(data2.n);
        //return data1.n.cross(data2.n);
    };

    auto resolveIntersectionType = [&]() {
        cout << " --- resolveIntersectionType" << N;
        for (auto r : roads) cout << "  " << r->getEntity()->getName();
        cout << endl;

        if (N == 2) {
            bool parallel  = bool( getRoadConnectionAngle(roads[0], roads[1]) < -0.8 );
            if (parallel) type = CONTINUATION;
        }

        if (N == 3) {
            bool parallel01 = bool(getRoadConnectionAngle(roads[0], roads[1]) < -0.5);
            bool parallel12 = bool(getRoadConnectionAngle(roads[1], roads[2]) < -0.5);
            bool parallel02 = bool(getRoadConnectionAngle(roads[2], roads[0]) < -0.5);
            if (parallel01 && parallel12 || parallel01 && parallel02 || parallel12 && parallel02) type = FORK;
            type = FORK;
        }

        auto entity = getEntity();
        switch (type) {
            case CONTINUATION: entity->set("type", "continuation"); break;
            case FORK: entity->set("type", "fork"); break;
            case MERGE: entity->set("type", "merge"); break;
            default: entity->set("type", "intersection");
        }
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

    auto elevateRoadNodes = [&]() {
        if (patch) {
            for (uint i=0; i<roads.size(); i++) { // elevate road nodes to median intersection height
                auto r = roads[i];
                auto e1 = r->getNodeEntry(node);
                auto n = e1->getEntity("node");
                auto d = e1->getVec3("direction");
                auto p = n->getVec3("position");
                p[1] = median[1];
                n->setVector("position", toStringVector(p), "Position");

                // check if any road noe is inside of the intersection!
                /*auto path = roads[i]->getEntity()->getEntity("path");
                for (auto e2 : path->getAllEntities("nodes")) {
                    if (e1 == e2) continue;
                    auto n = e2->getEntity("node");
                    auto np = n->getVec3f("position") - median;
                    if (patch->isInside(Vec2d(np[0],np[2]))) {
                        n->setVector("position", toStringVector(p-d*0.1), "Position");
                    }
                }*/
            }
        }
    };

    auto computeRoadFronts = [&]() {
        for (auto road : roads) { // compute road front
            auto& data = road->getEdgePoints( node );
            Vec3d p1 = data.p1;
            Vec3d p2 = data.p2;
            Vec3d norm = data.n;
            float d1 = (p1-pNode).dot(norm);
            float d2 = (p2-pNode).dot(norm);
            float d = min(d1,d2);
            d1 = max(0.f,d1-d);
            d2 = max(0.f,d2-d);
            data.p1 = p1-norm*(d1);
            data.p2 = p2-norm*(d2);

            Vec3d pm = (data.p1 + data.p2)*0.5; // compute road node
            int nID = graph->addNode();
            graph->setPosition(nID, pose::create(pm));
            auto n = addNode(nID, pm);
            data.entry->set("node", n->getName());
            n->add("paths", data.entry->getName());
            roadFronts.push_back( make_pair(pose(pm, norm), road->getWidth()) );
        }
    };

    auto computeIntersectionPaths = [&]() {
        vector<VREntityPtr> iPaths;
        for (uint i=0; i<roads.size(); i++) { // compute intersection paths
            auto road1 = roads[i];
            auto rEntry1 = road1->getNodeEntry(node);
            if (!rEntry1) continue;
            int s1 = toInt(rEntry1->get("sign")->value);
            Vec3d norm1 = rEntry1->getVec3("direction");
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
                    Vec3d norm2 = rEntry2->getVec3("direction");
                    auto& data2 = road2->getEdgePoints( node );
                    VREntityPtr node2 = data2.entry->getEntity("node");
                    auto pathEnt = addPath("Path", "intersection", {node1, node2}, {norm1, norm2});
                    iPaths.push_back(pathEnt);
                }
            }
        }
        for (auto path : iPaths) entity->add("path", path->getName());
    };

    auto resolveSpacialCases = [&]() {
        if (type == CONTINUATION) { // special cases for 2 roads
            /*bool parallel = bool(getRoadConnectionAngle(roads[0], roads[1]) < -0.8);
            if (parallel) { // nearly parallel, but opposite directions
                ; // TODO
                return true; // special case
            }*/
            return true;
        }

        if (type == FORK || type == MERGE) {
            /*Vec3d n1;
            for (int i=0; i<roads.size(); i++) {
                auto& data = roads[i]->getEdgePoints( node );
                if (n1.cross(data.n).squareLength() > 1e-5) return false; // not parallel, no special case, normal intersection
                n1 = data.n;
            }*/
            return true; // special case
        }

        return false; // no special case
    };

    sort( roads.begin(), roads.end(), compare );            // sort roads by how they are aligned next to each other
    resolveIntersectionType();
    if (!resolveSpacialCases()) resolveEdgeIntersections(); //
    computeRoadFronts();
    computeIntersectionPaths();
    computePatch();
    elevateRoadNodes();
}

