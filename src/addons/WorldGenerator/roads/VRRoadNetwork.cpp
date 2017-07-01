#include "VRRoadNetwork.h"
#include "VRRoad.h"
#include "VRRoadIntersection.h"
#include "../VRWorldGenerator.h"
#include "VRAsphalt.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/WorldGenerator/nature/VRWoods.h"
#include "core/tools/VRPathtool.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGMatrix22.h>

const double pi = 2*acos(0.0);

using namespace OSG;

VRRoadNetwork::VRRoadNetwork() : VRRoadBase("RoadNetwork") {}
VRRoadNetwork::~VRRoadNetwork() {}

VRRoadNetworkPtr VRRoadNetwork::create() {
    auto rn = VRRoadNetworkPtr( new VRRoadNetwork() );
    rn->init();
    return rn;
}

void VRRoadNetwork::init() {
    //tool = VRPathtool::create();
    asphalt = VRAsphalt::create();
    asphaltArrow = VRAsphalt::create();
    asphaltArrow->setArrowMaterial();

    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );
}

int VRRoadNetwork::getRoadID() { return ++nextRoadID; }
VRAsphaltPtr VRRoadNetwork::getMaterial() { return asphalt; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

void VRRoadNetwork::clear() {
	nextRoadID = 0;
	if (world->getOntology()) world->getOntology()->remEntities("RoadMarking");
	if (arrowTexture) arrowTexture = VRTexture::create();
    arrowTemplates.clear();
    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );
}

void VRRoadNetwork::updateAsphaltTexture() {
	asphalt->clearTexture();
	for (auto road : world->getOntology()->getEntities("Way")) {
        auto id = road->get("ID");
		if (!id) continue;
		int rID = toInt( id->value );

		auto markings = road->getAllEntities("markings");
		auto tracks = road->getAllEntities("tracks");

		for (auto marking : markings) {
            auto w = marking->get("width");
            auto dN = marking->get("dashNumber");
            float width = w ? toFloat( w->value ) : 0;
            int dashN = dN ? toInt( dN->value ) : 0;
            asphalt->addMarking(rID, toPath(marking, 16), width, dashN);
		}

		for (auto track : tracks) {
            auto w = track->get("width");
            auto dN = track->get("dashNumber");
            float width = w ? toFloat( w->value ) : 0;
            int dashN = dN ? toInt( dN->value ) : 0;
            asphalt->addTrack(rID, toPath(track, 16), width, dashN, trackWidth*0.5);
		}

		cout << "VRRoadNetwork::updateTexture, markings and tracks: " << markings.size() << " " << tracks.size() << endl;
	}
	asphalt->updateTexture();
}

VREntityPtr VRRoadNetwork::addGreenBelt( VREntityPtr road, float width ) {
	auto g = world->getOntology()->addEntity( road->getName()+"GreenBelt", "GreenBelt");
	g->set("width", toString(width));
	road->add("lanes", g->getName());
	return g;
}

VRRoadPtr VRRoadNetwork::addWay( string name, vector<VREntityPtr> paths, int rID, string type ) {
	auto roadEnt = world->getOntology()->addEntity( name+"Road", type );
	roadEnt->set("ID", toString(rID));
	for (auto path : paths) roadEnt->add("path", path->getName());
    auto road = VRRoad::create();
    road->setWorld(world);
    road->setEntity(roadEnt);
    addChild(road);
    ways.push_back(road);
	return road;
}

VRRoadPtr VRRoadNetwork::addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes ) {
    int rID = getRoadID();
    VREntityPtr pathEnt = addPath("Path", name, {node1, node2}, {norm1, norm2});
    VRRoadPtr road = addWay(name, {pathEnt}, rID, "Road");
    road->getEntity()->set("type", type);
    int Nm = Nlanes*0.5;
    for (int i=0; i<Nm; i++) road->addLane(1, 4 );
    for (int i=0; i<Nlanes-Nm; i++) road->addLane(-1, 4 );
    roads.push_back(road);
    return road;
}

VREntityPtr VRRoadNetwork::addArrows( VREntityPtr lane, float t, vector<float> dirs ) {
    auto arrow = world->getOntology()->addEntity("laneArrow", "Arrow");
    lane->add("arrows", arrow->getName());
    arrow->set("position", toString(t));
    arrow->set("lane", lane->getName());
    for (auto d : dirs) arrow->add("direction", toString(d));
    return arrow;
}

void VRRoadNetwork::addPole( Vec3f root, Vec3f end ) { // TODO
    ;
}

void VRRoadNetwork::computeLanePaths( VREntityPtr road ) {
    if (road->getAllEntities("path").size() > 1) cout << "Warning in VRRoadNetwork::computeLanePaths, road " << road->getName() << " has more than one path!\n";

    auto pathEnt = road->getEntity("path");
    if (!pathEnt) return;
	auto lanes = road->getAllEntities("lanes");

	float roadWidth = 0;
	for (auto lane : lanes) roadWidth += toFloat( lane->get("width")->value );

	float widthSum = -roadWidth*0.5;
	for (uint li=0; li<lanes.size(); li++) {
        auto lane = lanes[li];
		float width = toFloat( lane->get("width")->value );

		lane->clear("path");
		int direction = lane->get("direction") ? toInt( lane->get("direction")->value ) : 1;

        vector<VREntityPtr> nodes;
        vector<Vec3f> norms;

        for (auto entry : pathEnt->getAllEntities("nodes")) { // TODO: it might be more efficient to have this as outer loop
            Vec3f p = entry->getEntity("node")->getVec3f("position");
            Vec3f n = entry->getVec3f("direction");
            Vec3f x = Vec3f(0,1,0).cross(n);
            x.normalize();
            float k = width*0.5 + widthSum ;
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
		widthSum += width;
	}
}

vector<VREntityPtr> VRRoadNetwork::getRoadNodes() {
    //auto nodes = ontology->process("q(n):Node(n);Road(r);has(r.path.nodes,n)");
    map<int, VREntityPtr> nodes;
    for (auto road : roads) {
        for (auto p : road->getEntity()->getAllEntities("path")) {
            for (auto nE : p->getAllEntities("nodes")) {
                auto n = nE->getEntity("node");
                nodes[n->ID] = n;
            }
        }
    }
    vector<VREntityPtr> res;
    for (auto ni : nodes) res.push_back(ni.second);
    return res;
}

vector<VRRoadPtr> VRRoadNetwork::getNodeRoads(VREntityPtr node) {
    vector<VREntityPtr> nEntries;
    for (auto nE : node->getAllEntities("paths")) nEntries.push_back( nE );

    vector<VRRoadPtr> res; //= ontology->process("q(r):Node("+node->getName()+");Road(r);has(r.path.nodes,"+node->getName()+")");
    for (auto road : roads) {
        bool added = false;
        for (auto rp : road->getEntity()->getAllEntities("path")) {
            for (auto rnE : rp->getAllEntities("nodes")) {
                for (auto nE : nEntries) {
                    if (nE == rnE) { res.push_back(road); added = true; break; }
                }
                if (added) break;
            }
            if (added) break;
        }
    }
    return res;
}

void VRRoadNetwork::createArrow(Vec4i dirs, int N, const pose& p) {
    if (arrowTemplates.count(dirs) == 0) {
        arrowTemplates[dirs] = arrowTemplates.size();

        VRTextureGenerator tg;
        tg.setSize(Vec3i(400,400,1), true);
        tg.drawFill(Vec4f(0,0,1,1));

        for (int i=0; i<N; i++) {
            float a = dirs[i]*pi/180.0;
            Vec3f dir(sin(a), -cos(a), 0);
            Vec2f d02 = Vec2f(0.5,0.5); // rotation point
            Vec3f d03 = Vec3f(0.5,0.5,0); // rotation point

            auto apath = path::create();
            apath->addPoint( pose(Vec3f(0.5,1.0,0), Vec3f(0,-1,0), Vec3f(0,0,1)) );
            apath->addPoint( pose(Vec3f(0.5,0.8,0), Vec3f(0,-1,0), Vec3f(0,0,1)) );
            apath->addPoint( pose(d03+dir*0.31, dir, Vec3f(0,0,1)) );
            apath->compute(12);
            tg.drawPath(apath, Vec4f(1,1,1,1), 0.1);

            auto poly = VRPolygon::create();
            Matrix22<float> R = Matrix22<float>(cos(a), -sin(a), sin(a), cos(a));
            Vec2f A = Vec2f(0.35,0.2)-d02;
            Vec2f B = Vec2f(0.65,0.2)-d02;
            Vec2f C = Vec2f(0.5,0.0)-d02;
            A = R.mult(A); B = R.mult(B); C = R.mult(C);
            poly->addPoint(d02+A);
            poly->addPoint(d02+B);
            poly->addPoint(d02+C);
            tg.drawVRPolygon(poly, Vec4f(1,1,1,1));
        }

        auto aMask = tg.compose(0);
        if (!arrowTexture) arrowTexture = VRTexture::create();
        arrowTexture->merge(aMask, Vec3f(1,0,0));
        asphaltArrow->setTexture(arrowTexture);
        asphaltArrow->setShaderParameter("NArrowTex", (int)arrowTemplates.size());
    }

    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
    Vec4f color = Vec4f((arrowTemplates[dirs]-1)*0.001, 0, 0);
    for (int i=0; i<4; i++) cols->addValue(color);

    VRGeoData gdata;
    gdata.pushQuad(Vec3f(0,0.02,0), Vec3f(0,1,0), Vec3f(0,0,1), Vec2f(2,2), true);
    auto geo = gdata.asGeometry("arrow");
    geo->setPose( pose::create(p) );
    geo->applyTransformation();
    geo->setColors(cols);
    geo->setPositionalTexCoords2D(1.0, 1, Vec2i(0,2));
    arrows->merge(geo);
    return;
}

void VRRoadNetwork::computeIntersections() {
    int k = 0;
    for (auto node : getRoadNodes() ) {
        Vec3f pNode = node->getVec3f("position");
        auto nodeRoads = getNodeRoads(node);
        if (nodeRoads.size() <= 2) continue; // for now ignore ends and curves

        // sort nodeRoads
        auto compare = [&](VRRoadPtr road1, VRRoadPtr road2) -> bool {
            Vec3f norm1 = road1->getEdgePoints( node ).n;
            Vec3f norm2 = road2->getEdgePoints( node ).n;
            float K = norm1.cross(norm2)[1];
            return (K < 0);
        };

        sort( nodeRoads.begin(), nodeRoads.end(), compare );

        auto intersect = [&](const Pnt3f& p1, const Vec3f& n1, const Pnt3f& p2, const Vec3f& n2) -> Vec3f {
            Vec3f d = p2-p1;
            Vec3f n3 = n1.cross(n2);
            float N3 = n3.dot(n3);
            if (N3 == 0) N3 = 1.0;
            float s = d.cross(n2).dot(n1.cross(n2))/N3;
            return Vec3f(p1) + n1*s;
        };

        int N = nodeRoads.size();
        for (int r = 0; r<N; r++) { // compute intersection points
            auto road1 = nodeRoads[r];
            auto road2 = nodeRoads[(r+1)%N];
            auto& data1 = road1->getEdgePoints( node );
            auto& data2 = road2->getEdgePoints( node );
            Vec3f Pi = intersect(data1.p2, data1.n, data2.p1, data2.n);
            data1.p2 = Pi;
            data2.p1 = Pi;
        }

        for (auto road : nodeRoads) { // compute road front
            auto& data = road->getEdgePoints( node );
            Vec3f p1 = data.p1;
            Vec3f p2 = data.p2;
            Vec3f norm = data.n;
            float d1 = abs((p1-pNode).dot(norm));
            float d2 = abs((p2-pNode).dot(norm));
            float d = max(d1,d2);
            data.p1 = p1-norm*(d-d1);
            data.p2 = p2-norm*(d-d2);

            Vec3f pm = (data.p1 + data.p2)*0.5; // compute road node
            auto n = addNode(pm);
            data.entry->set("node", n->getName());
            n->add("paths", data.entry->getName());
            k += 1;
        }

        vector<VREntityPtr> iPaths;
        for (uint i=0; i<nodeRoads.size(); i++) { // compute intersection paths
            auto road1 = nodeRoads[i];
            auto rEntry1 = road1->getNodeEntry(node);
            if (!rEntry1) continue;
            int s1 = toInt(rEntry1->get("sign")->value);
            Vec3f norm1 = rEntry1->getVec3f("direction");
            auto& data1 = road1->getEdgePoints( node );
            VREntityPtr node1 = data1.entry->getEntity("node");
            if (s1 == 1) {
                for (uint j=0; j<nodeRoads.size(); j++) { // compute intersection paths
                    auto road2 = nodeRoads[j];
                    if (j == i) continue;
                    auto rEntry2 = road2->getNodeEntry(node);
                    if (!rEntry2) continue;
                    int s2 = toInt(rEntry2->get("sign")->value);
                    if (s2 != -1) continue;
                    Vec3f norm2 = rEntry2->getVec3f("direction");
                    auto& data2 = road2->getEdgePoints( node );
                    VREntityPtr node2 = data2.entry->getEntity("node");
                    auto pathEnt = addPath("Path", "intersection", {node1, node2}, {norm1, norm2});
                    iPaths.push_back(pathEnt);
                }
            }
        }

        int rID = getRoadID();
        auto iEnt = world->getOntology()->addEntity( "intersectionRoad", "RoadIntersection" );
        iEnt->set("ID", toString(rID));
        for (auto path : iPaths) iEnt->add("path", path->getName());
        auto intersection = VRRoadIntersection::create();
        intersection->setWorld(world);
        intersection->setEntity(iEnt);
        intersections.push_back(intersection);
        addChild(intersection);
        for (auto r : nodeRoads) iEnt->add("roads", r->getEntity()->getName());
        iEnt->set("node", node->getName());
    }
}

void VRRoadNetwork::computeTracksLanes(VREntityPtr way) {
    auto getBulge = [&](vector<pose>& points, uint i, Vec3f& x) -> float {
        if (points.size() < 2) return 0;
        if (i == 0) return 0;
        if (i == points.size()-1) return 0;

        Vec3f t1 = points[i-1].pos() - points[i].pos();
        t1.normalize();
        Vec3f t2 = points[i+1].pos() - points[i].pos();
        t2.normalize();

        float b1 = -x.dot(t1);
        float b2 = -x.dot(t2);
        return (b1+b2)*trackWidth*0.5;
    };

    for (auto lane : way->getAllEntities("lanes")) {
        if (!lane->is_a("Lane")) continue;
        for (auto pathEnt : lane->getAllEntities("path")) {
            auto path = toPath(pathEnt, 2);
            vector<VREntityPtr> nodes;
            vector<Vec3f> normals;
            auto points = path->getPoints();
            for (uint i=0; i < points.size(); i++) {
                auto& point = points[i];
                Vec3f p = point.pos();
                Vec3f n = point.dir();
                float nL = n.length();
                Vec3f x = n.cross(Vec3f(0,1,0));
                x.normalize();
                float bulge = getBulge(points, i, x);
                p += x*bulge;
                n -= x*bulge*0.1; n.normalize(); n *= nL;
                nodes.push_back( addNode(p) );
                normals.push_back( n );
            }
            auto t = addPath("RoadTrack", name, nodes, normals);
            t->set("width", toString(trackWidth*0.5));
            way->add("tracks", t->getName());
        }
    }
}

void VRRoadNetwork::setNatureManager(VRWoodsPtr mgr) { natureManager = mgr; }

// --------------- pipeline -----------------------

void VRRoadNetwork::computeLanes() {
    for (auto road : world->getOntology()->getEntities("Road")) computeLanePaths(road);
    for (auto intersection : intersections) {
        intersection->computeLanes();
        intersection->computeTrafficLights();
    }
}

void VRRoadNetwork::computeSurfaces() {
    auto computeRoadSurface = [&](VRRoadPtr road) {
        auto roadGeo = road->createGeometry();
        roadGeo->setMaterial( asphalt );
        if (!roadGeo) return;
        roadGeo->hide();
        roadGeo->getPhysics()->setDynamic(false);
        roadGeo->getPhysics()->setShape("Concave");
        roadGeo->getPhysics()->setPhysicalized(true);
        addChild( roadGeo );
    };

    for (auto way : ways) computeRoadSurface(way);
    for (auto road : roads) computeRoadSurface(road);

    for (auto intersection : intersections) {
        auto iGeo = intersection->createGeometry();
        if (!iGeo) continue;
        iGeo->setMaterial( asphalt );
        iGeo->hide();
        addChild( iGeo );
    }

    for (auto arrow : world->getOntology()->getEntities("Arrow")) {
        float t = toFloat( arrow->get("position")->value );
        auto lane = arrow->getEntity("lane");
        if (!lane || !lane->getEntity("path")) continue;
        auto lpath = toPath( lane->getEntity("path"), 32 );
        auto dirs = arrow->getAll("direction");
        Vec4i drs(999,999,999,999);
        for (uint i=0; i<4 && i < dirs.size(); i++) drs[i] = toFloat(dirs[i]->value)*180/pi;
        createArrow(drs, dirs.size(), lpath->getPose(t));
    }
}

void VRRoadNetwork::computeMarkings() {
    for (auto way : world->getOntology()->getEntities("Way")) computeTracksLanes(way);
    for (auto road : roads) {
        string type = "residential";
        if (auto t = road->getEntity()->get("type")) type = t->value;
        if (type != "unclassified") road->computeMarkings2();
    }
    for (auto road : ways) {
        string type = "residential";
        if (auto t = road->getEntity()->get("type")) type = t->value;
        if (type != "unclassified") road->computeMarkings2();
    }
    for (auto intersection : intersections) intersection->computeMarkings();
}

vector<VRPolygonPtr> VRRoadNetwork::computeGreenBelts() {
    vector<VRPolygonPtr> areas;
    for (auto belt : world->getOntology()->getEntities("GreenBelt")) {
        VRPolygonPtr area = VRPolygon::create();
        for (auto pathEnt : belt->getAllEntities("path")) {
            float width = toFloat( belt->get("width")->value ) * 0.3;
            vector<Vec3f> rightPoints;
            for (auto& point : toPath(pathEnt, 16)->getPoses()) {
                Vec3f p = point.pos();
                Vec3f n = point.dir();
                Vec3f x = point.x();
                Vec3f pL = p - x*width;
                Vec3f pR = p + x*width;
                area->addPoint( Vec2f(pL[0],pL[2]) );
                rightPoints.push_back(pR);
            }
            for (auto p = rightPoints.rbegin(); p != rightPoints.rend(); p++) area->addPoint( Vec2f((*p)[0],(*p)[2]) );
        }

        areas.push_back(area);

        /*if (natureManager) {
            natureManager->addGrassPatch(area, false);
            auto pntsB = area->getRandomPoints(1, 0);
            auto pntsT = area->getRandomPoints(0.1, 0);
            for (p : pntsB) natureManager->addTree(p);
            for (p : pntsT) natureManager->addTree(p);
        }*/
    }
    return areas;
}

void VRRoadNetwork::compute() {
    computeIntersections();
    computeLanes();
    computeSurfaces();
    computeMarkings();
    //computeGreenBelts();
}





