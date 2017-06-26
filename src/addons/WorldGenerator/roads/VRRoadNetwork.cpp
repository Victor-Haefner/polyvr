#include "VRRoadNetwork.h"
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



class Road {
    private:
        struct edgePoint {
            Vec3f p1;
            Vec3f p2;
            Vec3f n;
            VREntityPtr entry;

            edgePoint() {}
            edgePoint(Vec3f p1, Vec3f p2, Vec3f n, VREntityPtr e) : p1(p1), p2(p2), n(n), entry(e) {}
        };

        VREntityPtr entity;
        map<VREntityPtr, edgePoint> edgePoints;

    public:
        Road() {}
        Road( VREntityPtr e ) : entity(e) {}

        float getWidth() {
            float width = 0;
            for (auto lane : entity->getAllEntities("lanes")) width += toFloat( lane->get("width")->value );
            return width;
        }

        VREntityPtr getNodeEntry( VREntityPtr node ) {
            /*string rN = entity->getName();
            string nN = node->getName();
            auto nodeEntry = entity->ontology.lock()->process("q(e):NodeEntry(e);Node("+nN+");Road("+rN+");has("+rN+".path,e);has("+nN+",e)");
            return nodeEntry[0];*/

            for (auto rp : entity->getAllEntities("path")) {
                for (auto rnE : rp->getAllEntities("nodes")) {
                    for (auto nE : node->getAllEntities("paths")) {
                        if (rnE == nE) return nE;
                    }
                }
            }
            return 0;
        }

        edgePoint& getEdgePoints( VREntityPtr node ) {
            if (edgePoints.count(node) == 0) {
                float width = getWidth();
                VREntityPtr rEntry = getNodeEntry( node );
                Vec3f norm = rEntry->getVec3f("direction") * toInt(rEntry->get("sign")->value);
                Vec3f x = Vec3f(0,1,0).cross(norm);
                x.normalize();
                Vec3f pNode = node->getVec3f("position");
                Vec3f p1 = pNode - x * 0.5 * width; // right
                Vec3f p2 = pNode + x * 0.5 * width; // left
                edgePoints[node] = edgePoint(p1,p2,norm,rEntry);
            }
            return edgePoints[node];
        }

};



VRRoadNetwork::VRRoadNetwork() : VRObject("RoadNetwork") {}
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
void VRRoadNetwork::setOntology(VROntologyPtr o) { ontology = o; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

void VRRoadNetwork::clear() {
	nextRoadID = 0;
	if (ontology) ontology->remEntities("RoadMarking");
	if (arrowTexture) arrowTexture = VRTexture::create();
    arrowTemplates.clear();
    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );
}

void VRRoadNetwork::updateAsphaltTexture() {
	asphalt->clearTexture();
	for (auto road : ontology->getEntities("Way")) {
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

vector<string> toStringVector(Vec3f& v) {
    vector<string> res;
    res.push_back( toString(v[0]) );
    res.push_back( toString(v[1]) );
    res.push_back( toString(v[2]) );
    return res;
}

VREntityPtr VRRoadNetwork::addNode( Vec3f pos ) {
	auto node = ontology->addEntity("node", "Node");
	node->setVector("position", toStringVector(pos), "Position");

	if (tool) {
        int nID = tool->addNode( pose::create(pos, Vec3f(0,0,-1), Vec3f(0,1,0) ) );
        auto handle = tool->getHandle(nID);
        handle->setEntity(node);
        node->set("graphID", toString(nID) );
	}
	return node;
}

VREntityPtr VRRoadNetwork::addLane( int direction, VREntityPtr road, float width ) {
	auto l = ontology->addEntity( road->getName()+"Lane", "Lane");
	l->set("width", toString(width));
	l->set("direction", toString(direction));
	road->add("lanes", l->getName());
	return l;
}

VREntityPtr VRRoadNetwork::addGreenBelt( VREntityPtr road, float width ) {
	auto g = ontology->addEntity( road->getName()+"GreenBelt", "GreenBelt");
	g->set("width", toString(width));
	road->add("lanes", g->getName());
	return g;
}

VREntityPtr VRRoadNetwork::addWay( string name, vector<VREntityPtr> paths, int rID, string type ) {
	auto r = ontology->addEntity( name+"Road", type );
	r->set("ID", toString(rID));
	for (auto path : paths) r->add("path", path->getName());
	return r;
}

VREntityPtr VRRoadNetwork::addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes ) {
    int rID = getRoadID();
    VREntityPtr pathEnt = addPath("Path", name, {node1, node2}, {norm1, norm2});
    VREntityPtr roadEnt = addWay(name, {pathEnt}, rID, "Road");
    roadEnt->set("type", type);
    int Nm = Nlanes*0.5;
    for (int i=0; i<Nm; i++) addLane(1, roadEnt, 4 );
    for (int i=0; i<Nlanes-Nm; i++) addLane(-1, roadEnt, 4 );
    roads.push_back(roadEnt);
    return roadEnt;
}

VREntityPtr VRRoadNetwork::addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3f> normals ) {
    auto path = ontology->addEntity(name+"Path", type);
	VREntityPtr lastNode;
	Vec3f nL;
	int N = nodes.size();

	for ( int i = 0; i< N; i++) {
        auto node = nodes[i];
        if (!node) { cout << "Warning in VRRoadNetwork::addPath, NULL node!" << endl; continue; }
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

		if (lastNode && tool) {
			int nID1 = toInt(lastNode->get("graphID")->value);
			int nID2 = toInt(node->get("graphID")->value);
			tool->connect(nID1, nID2, 1, nL, norm);
		}
		lastNode = node;
		nL = norm;
	}

	return path;
}

VREntityPtr VRRoadNetwork::addArrows( VREntityPtr lane, float t, vector<float> dirs ) {
    auto arrow = ontology->addEntity("laneArrow", "Arrow");
    lane->add("arrows", arrow->getName());
    arrow->set("position", toString(t));
    arrow->set("lane", lane->getName());
    for (auto d : dirs) arrow->add("direction", toString(d));
    return arrow;
}

void VRRoadNetwork::computeIntersectionLanes( VREntityPtr intersection ) {
	vector<VREntityPtr> roads = intersection->getAllEntities("roads");
	VREntityPtr node = intersection->getEntity("node");
	if (!node) { cout << "Warning in VRRoadNetwork::computeIntersectionLanes, intersection node is NULL!" << endl; return; }
	string iN = intersection->getName();
	string nN = node->getName();

	// get in and out lanes
	vector< pair<VREntityPtr, VREntityPtr> > inLanes;
	vector< pair<VREntityPtr, VREntityPtr> > outLanes;
	for (VREntityPtr road : roads) {
        Road r(road);
		VREntityPtr roadEntry = r.getNodeEntry(node);
		int reSign = toInt( roadEntry->get("sign")->value );
		for (VREntityPtr lane : road->getAllEntities("lanes")) {
            if (!lane->is_a("Lane")) continue;
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
    if (!pathEntity) return 0;
	vector<Vec3f> pos;
	vector<Vec3f> norms;
	for (auto nodeEntry : pathEntity->getAllEntities("nodes")) {
        auto node = nodeEntry->getEntity("node");
        if (!node) { cout << "Warning in VRRoadNetwork::toPath, path node is NULL!" << endl; continue; }
		pos.push_back( node->getVec3f("position") );
		norms.push_back(  nodeEntry->getVec3f("direction") );
	}

	pathPtr Path = path::create();
	for (uint i=0; i<pos.size(); i++) Path->addPoint(pose(pos[i], norms[i]));
	Path->compute(resolution);
	return Path;
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

void VRRoadNetwork::setupTexCoords( VRGeometryPtr geo, VREntityPtr way ) {
	int rID = toInt( way->get("ID")->value );
	GeoVec2fPropertyRecPtr tcs = GeoVec2fProperty::create();
	for (int i=0; i<geo->size(); i++) tcs->addValue(Vec2f(rID, 0));
	geo->setPositionalTexCoords2D(1.0, 0, Vec2i(0,2)); // positional tex coords
	geo->setTexCoords(tcs, 1); // add another tex coords set
	geo->setMaterial( asphalt );
}

VRGeometryPtr VRRoadNetwork::createRoadGeometry( VREntityPtr roadEnt ) {
	auto strokeGeometry = [&]() {
	    Road roadd(roadEnt);
	    float width = roadd.getWidth();
		float W = width*0.5*1.1;
		vector<Vec3f> profile;
		profile.push_back(Vec3f(-W,0,0));
		profile.push_back(Vec3f(W,0,0));

		auto road = VRStroke::create("road");
		vector<pathPtr> paths;
		for (auto p : roadEnt->getAllEntities("path")) {
            paths.push_back( toPath(p,64) );
		}
		road->setPaths( paths );
		road->strokeProfile(profile, 0, 0);
		return road;
	};

	auto road = strokeGeometry();
	setupTexCoords( road, roadEnt );
	return road;
}

vector<VREntityPtr> VRRoadNetwork::getRoadNodes() {
    //auto nodes = ontology->process("q(n):Node(n);Road(r);has(r.path.nodes,n)");
    map<int, VREntityPtr> nodes;
    for (auto r : roads) {
        for (auto p : r->getAllEntities("path")) {
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

vector<VREntityPtr> VRRoadNetwork::getNodeRoads(VREntityPtr node) {
    vector<VREntityPtr> nEntries;
    for (auto nE : node->getAllEntities("paths")) nEntries.push_back( nE );

    vector<VREntityPtr> res; //= ontology->process("q(r):Node("+node->getName()+");Road(r);has(r.path.nodes,"+node->getName()+")");
    for (auto r : roads) {
        bool added = false;
        for (auto rp : r->getAllEntities("path")) {
            for (auto rnE : rp->getAllEntities("nodes")) {
                for (auto nE : nEntries) {
                    if (nE == rnE) { res.push_back(r); added = true; break; }
                }
                if (added) break;
            }
            if (added) break;
        }
    }
    return res;
}

VREntityPtr VRRoadNetwork::getIntersectionRoadNode(VREntityPtr roadEnt, VREntityPtr intersectionEnt) {
    /*string iN = intersectionEnt->getName();
    string rN = roadEnt->getName();
    auto res = ontology->process("q(n):Node(n);Road("+rN+");RoadIntersection("+iN+");has("+rN+".path.nodes,n);has("+iN+".path.nodes,n)");
    if (res.size() == 0) { cout << "Warning in createIntersectionGeometry, road " << roadEnt->getName() << " has no nodes!" << endl; return 0; }
    return res[0];*/

    for (auto rp : roadEnt->getAllEntities("path")) {
        for (auto rnE : rp->getAllEntities("nodes")) {
            for (auto ip : intersectionEnt->getAllEntities("path")) {
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

VRGeometryPtr VRRoadNetwork::createIntersectionGeometry( VREntityPtr intersectionEnt ) {
    VRPolygon poly; // intersection VRPolygon
    auto roads = intersectionEnt->getAllEntities("roads");
    VREntityPtr node = intersectionEnt->getEntity("node");
    if (!node) return 0;
    for (auto roadEnt : roads) {
        Road road(roadEnt);
        auto rNode = getIntersectionRoadNode(roadEnt, intersectionEnt);
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
	setupTexCoords( intersection, intersectionEnt );
	return intersection;
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
        vector<VREntityPtr> roads = getNodeRoads(node);

        if (roads.size() <= 2) continue; // for now ignore ends and curves
        map<VREntityPtr, Road> roadData;
        for (VREntityPtr roadEnt : roads) roadData[roadEnt] = Road(roadEnt);

        // sort roads
        auto compare = [&](VREntityPtr road1, VREntityPtr road2) -> bool {
            Vec3f norm1 = roadData[road1].getEdgePoints( node ).n;
            Vec3f norm2 = roadData[road2].getEdgePoints( node ).n;
            float K = norm1.cross(norm2)[1];
            return (K < 0);
        };

        sort( roads.begin(), roads.end(), compare );

        auto intersect = [&](const Pnt3f& p1, const Vec3f& n1, const Pnt3f& p2, const Vec3f& n2) -> Vec3f {
            Vec3f d = p2-p1;
            Vec3f n3 = n1.cross(n2);
            float N3 = n3.dot(n3);
            if (N3 == 0) N3 = 1.0;
            float s = d.cross(n2).dot(n1.cross(n2))/N3;
            return Vec3f(p1) + n1*s;
        };

        int N = roads.size();
        for (int r = 0; r<N; r++) { // compute intersection points
            VREntityPtr roadEnt1 = roads[r];
            VREntityPtr roadEnt2 = roads[(r+1)%N];
            auto& data1 = roadData[roadEnt1].getEdgePoints( node );
            auto& data2 = roadData[roadEnt2].getEdgePoints( node );
            Vec3f Pi = intersect(data1.p2, data1.n, data2.p1, data2.n);
            data1.p2 = Pi;
            data2.p1 = Pi;
        }

        for (auto roadEnt : roads) { // compute road front
            auto& data = roadData[roadEnt].getEdgePoints( node );
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
        for (uint i=0; i<roads.size(); i++) { // compute intersection paths
            auto road1 = roads[i];
            auto rEntry1 = roadData[road1].getNodeEntry(node);
            if (!rEntry1) continue;
            int s1 = toInt(rEntry1->get("sign")->value);
            Vec3f norm1 = rEntry1->getVec3f("direction");
            auto& data1 = roadData[road1].getEdgePoints( node );
            VREntityPtr node1 = data1.entry->getEntity("node");
            if (s1 == 1) {
                for (uint j=0; j<roads.size(); j++) { // compute intersection paths
                    auto road2 = roads[j];
                    if (j == i) continue;
                    auto rEntry2 = roadData[road2].getNodeEntry(node);
                    if (!rEntry2) continue;
                    int s2 = toInt(rEntry2->get("sign")->value);
                    if (s2 != -1) continue;
                    Vec3f norm2 = rEntry2->getVec3f("direction");
                    auto& data2 = roadData[road2].getEdgePoints( node );
                    VREntityPtr node2 = data2.entry->getEntity("node");
                    auto pathEnt = addPath("Path", "intersection", {node1, node2}, {norm1, norm2});
                    iPaths.push_back(pathEnt);
                }
            }
        }

        int rID = getRoadID();
        auto intersection = addWay("intersection", iPaths, rID, "RoadIntersection");
        for (auto r : roads) intersection->add("roads", r->getName());
        intersection->set("node", node->getName());
    }
}

void VRRoadNetwork::computeMarkingsRoad2(VREntityPtr roadEnt) {
    float mw = 0.15;
    Road road(roadEnt);

    // road data
    vector<VREntityPtr> nodes;
    vector<Vec3f> normals;
    VREntityPtr pathEnt = roadEnt->getEntity("path");
    if (!pathEnt) return;

    VREntityPtr nodeEntryIn = pathEnt->getEntity("nodes",0);
    VREntityPtr nodeEntryOut = pathEnt->getEntity("nodes",1);
    VREntityPtr nodeIn = nodeEntryIn->getEntity("node");
    VREntityPtr nodeOut = nodeEntryOut->getEntity("node");
    Vec3f normIn = nodeEntryIn->getVec3f("direction");
    Vec3f normOut = nodeEntryOut->getVec3f("direction");

    float roadWidth = road.getWidth();
    auto lanes = roadEnt->getAllEntities("lanes");
    int Nlanes = lanes.size();

    // compute markings nodes
    auto path = toPath(pathEnt, 12);
    for (auto point : path->getPoints()) {
        Vec3f p = point.pos();
        Vec3f n = point.dir();
        Vec3f x = point.x();
        x.normalize();

        float widthSum = -roadWidth*0.5;
        for (int li=0; li<Nlanes; li++) {
            auto lane = lanes[li];
            float width = toFloat( lane->get("width")->value );
            float k = widthSum + mw*0.5;

            Vec3f pi = x*k + p;
            nodes.push_back(addNode(pi));
            normals.push_back(n);
            widthSum += width;
        }
        nodes.push_back(addNode(x*(roadWidth*0.5 - mw*0.5) + p));
        normals.push_back(n);
    }

    // markings
    int pathN = path->size();
    float L = path->getLength();
    string Ndots = toString(int(L*0.5));
    int lastDir = 0;
    for (int li=0; li<Nlanes+1; li++) {
        vector<VREntityPtr> nodes2;
        vector<Vec3f> normals2;
        for (int pi=0; pi<pathN; pi++) {
            int i = pi*(Nlanes+1)+li;
            nodes2.push_back( nodes[i] );
            normals2.push_back( normals[i] );
        }
        auto mL = addPath("RoadMarking", name, nodes2, normals2);
        mL->set("width", toString(mw));
        roadEnt->add("markings", mL->getName());

        if (li != Nlanes) {
            auto lane = lanes[li];
            if (!lane->is_a("Lane")) { lastDir = 0; continue; }
            int direction = toInt( lane->get("direction")->value );
            if (li != 0 && lastDir*direction > 0) {
                mL->set("style", "dashed");
                mL->set("dashNumber", Ndots);
            }
            lastDir = direction;
        }
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

void VRRoadNetwork::computeMarkingsIntersection( VREntityPtr intersection) {
    string name = intersection->getName();

    map<VREntityPtr, float> laneEntries;
    for (auto lane : intersection->getAllEntities("lanes")) {
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
		intersection->add("markings", m->getName());
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

void VRRoadNetwork::setNatureManager(VRWoodsPtr mgr) { natureManager = mgr; }

// --------------- pipeline -----------------------

void VRRoadNetwork::computeLanes() {
    for (auto road : ontology->getEntities("Road")) computeLanePaths(road);
    for (auto intersection : ontology->getEntities("RoadIntersection")) computeIntersectionLanes(intersection);
}

void VRRoadNetwork::computeSurfaces() {
    for (auto road : ontology->getEntities("Road")) {
        auto roadGeo = createRoadGeometry( road );
        if (!roadGeo) continue;
        roadGeo->hide();
        roadGeo->getPhysics()->setDynamic(false);
        roadGeo->getPhysics()->setShape("Concave");
        roadGeo->getPhysics()->setPhysicalized(true);
        addChild( roadGeo );
    }

    for (auto intersection : ontology->getEntities("RoadIntersection")) {
        auto iGeo = createIntersectionGeometry( intersection );
        if (!iGeo) continue;
        iGeo->hide();
        addChild( iGeo );
    }

    for (auto arrow : ontology->getEntities("Arrow")) {
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
    for (auto way : ontology->getEntities("Road")) computeTracksLanes(way); // Way
    for (auto road : ontology->getEntities("Road")) {
        string type = "residential";
        if (auto t = road->get("type")) type = t->value;
        if (type != "unclassified") computeMarkingsRoad2( road );
    }
    for (auto intersection : ontology->getEntities("RoadIntersection")) computeMarkingsIntersection( intersection );
}

vector<VRPolygonPtr> VRRoadNetwork::computeGreenBelts() {
    vector<VRPolygonPtr> areas;
    for (auto belt : ontology->getEntities("GreenBelt")) {
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





