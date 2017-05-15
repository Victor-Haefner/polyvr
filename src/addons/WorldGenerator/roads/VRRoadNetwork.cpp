#include "VRRoadNetwork.h"
#include "VRAsphalt.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/tools/VRPathtool.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>

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

        VREntityPtr getEntry( VREntityPtr node ) {
            string rN = entity->getName();
            string nN = node->getName();
            auto nodeEntry = entity->ontology.lock()->process("q(e):NodeEntry(e);Node("+nN+");Road("+rN+");has("+rN+".path,e);has("+nN+",e)");
            return nodeEntry[0];
        }

        edgePoint& getEdgePoints( VREntityPtr node ) {
            if (edgePoints.count(node) == 0) {
                float width = getWidth();
                VREntityPtr rEntry = getEntry( node );
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



VRRoadNetwork::VRRoadNetwork() : VRObject("RoadNetwork") {
    tool = VRPathtool::create();
    asphalt = VRAsphalt::create();
}

VRRoadNetwork::~VRRoadNetwork() {}

VRRoadNetworkPtr VRRoadNetwork::create() { return VRRoadNetworkPtr( new VRRoadNetwork() ); }

int VRRoadNetwork::getRoadID() { return ++nextRoadID; }
VRAsphaltPtr VRRoadNetwork::getMaterial() { return asphalt; }
void VRRoadNetwork::setOntology(VROntologyPtr o) { ontology = o; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

void VRRoadNetwork::clear() {
	nextRoadID = 0;
	if (ontology) ontology->remEntities("RoadMarking");
    //if (asphalt) asphalt = VRAsphalt::create();
}

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

VREntityPtr VRRoadNetwork::addWay( string name, vector<VREntityPtr> paths, int rID, string type ) {
	auto r = ontology->addEntity( name+"Road", type );
	r->set("ID", toString(rID));
	for (auto path : paths) r->add("path", path->getName());
	return r;
}

void VRRoadNetwork::addRoad( string name, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes ) {
    int rID = getRoadID();
    vector<VREntityPtr> nodes; nodes.push_back(node1); nodes.push_back(node2);
    vector<Vec3f> norms; norms.push_back(norm1); norms.push_back(norm2);
    VREntityPtr pathEnt = addPath("Path", name, nodes, norms);
    vector<VREntityPtr> paths; paths.push_back(pathEnt);
    VREntityPtr roadEnt = addWay(name, paths, rID, "Road");
    int Nm = Nlanes*0.5;
    for (int i=0; i<Nm; i++) addLane(1, roadEnt, 4 );
    for (int i=0; i<Nlanes-Nm; i++) addLane(-1, roadEnt, 4 );
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

VREntityPtr VRRoadNetwork::addPath( string type, string name, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2 ) {
    vector<VREntityPtr> nodes; nodes.push_back(node1); nodes.push_back(node2);
    vector<Vec3f> norms; norms.push_back(norm1); norms.push_back(norm2);
    return addPath(type, name, nodes, norms);
}

void VRRoadNetwork::computeIntersectionLanes( VREntityPtr intersection ) {
	vector<VREntityPtr> roads = intersection->getAllEntities("roads");
	VREntityPtr node = intersection->getEntity("node");
	if (!node) { cout << "Warning in VRRoadNetwork::computeIntersectionLanes, intersection node is NULL!" << endl; return; }
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
        auto node = nodeEntry->getEntity("node");
        if (!node) { cout << "Warning in VRRoadNetwork::toPath, path node is NULL!" << endl; continue; }
		pos.push_back( node->getVec3f("position") );
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
            paths.push_back( toPath(p,16) );
		}
		road->setPaths( paths );
		road->strokeProfile(profile, 0, 0);
		return road;
	};

	auto road = strokeGeometry();
	setupTexCoords( road, roadEnt );
	return road;
}

VRGeometryPtr VRRoadNetwork::createIntersectionGeometry( VREntityPtr intersectionEnt ) {
    polygon poly; // intersection polygon
    auto roads = intersectionEnt->getAllEntities("roads");
    VREntityPtr node = intersectionEnt->getEntity("node");
    if (!node) return 0;
    string iN = intersectionEnt->getName();
    string nN = node->getName();
    for (auto roadEnt : roads) {
        Road road(roadEnt);
        string rN = roadEnt->getName();
        string qNode = "q(n):Node(n);Road("+rN+");RoadIntersection("+iN+");has("+rN+".path.nodes,n);has("+iN+".path.nodes,n)";
        auto rNodes = ontology->process(qNode);
        if (rNodes.size() == 0) { cout << "Warning in createIntersectionGeometry, road " << roadEnt->getName() << " has no nodes!" << endl; continue; }
        auto& endP = road.getEdgePoints( rNodes[0] );
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

void VRRoadNetwork::computeIntersections() {
    int k = 0;
    for (auto node : ontology->process("q(n):Node(n);Road(r);has(r.path.nodes,n)") ) {
        Vec3f pNode = node->getVec3f("position");
        vector<VREntityPtr> roads = ontology->process("q(r):Node("+node->getName()+");Road(r);has(r.path.nodes,"+node->getName()+")");
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
        for (int i=0; i<roads.size(); i++) { // compute intersection paths
            auto road1 = roads[i];
            auto rEntry1 = roadData[road1].getEntry(node);
            int s1 = toInt(rEntry1->get("sign")->value);
            Vec3f norm1 = rEntry1->getVec3f("direction");
            auto& data1 = roadData[road1].getEdgePoints( node );
            VREntityPtr node1 = data1.entry->getEntity("node");
            if (s1 == 1) {
                for (int j=0; j<roads.size(); j++) { // compute intersection paths
                    auto road2 = roads[j];
                    if (j == i) continue;
                    auto rEntry2 = roadData[road2].getEntry(node);
                    int s2 = toInt(rEntry2->get("sign")->value);
                    if (s2 != -1) continue;
                    Vec3f norm2 = rEntry2->getVec3f("direction");
                    auto& data2 = roadData[road2].getEdgePoints( node );
                    VREntityPtr node2 = data2.entry->getEntity("node");
                    auto pathEnt = addPath("Path", "intersection", node1, node2, norm1, norm2);
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

    computeTracksLanes(roadEnt);

    // road data
    vector<VREntityPtr> nodes;
    vector<Vec3f> normals;
    VREntityPtr pathEnt = roadEnt->getEntity("path");
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
        Vec3f x = n.cross(Vec3f(0,1,0));
        x.normalize();

        for (int li=0; li<Nlanes; li++) {
            auto lane = lanes[li];
            float width = toFloat( lane->get("width")->value );
            float k = width*li - roadWidth*0.5 + mw*0.5;
            Vec3f pi = x*k + p;
            nodes.push_back(addNode(pi));
            normals.push_back(n);
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
    for (auto lane : way->getAllEntities("lanes")) {
        for (auto pathEnt : lane->getAllEntities("path")) {
            // tracks
            auto path = toPath(pathEnt, 12);
            vector<VREntityPtr> nodes;
            vector<Vec3f> normals;
            for (auto point : path->getPoints()) {
                Vec3f p = point.pos();
                Vec3f n = point.dir();
                Vec3f x = n.cross(Vec3f(0,1,0));
                x.normalize();
                Vec3f pL = p + x*(-trackWidth*0.5);
                Vec3f pR = p + x*( trackWidth*0.5);
                nodes.push_back( addNode(pL) );
                nodes.push_back( addNode(pR) );
                //for (int i : {0,1}) normals.push_back(n);
                normals.insert(normals.end(), 2, n);
            }
            auto tL = addPath("RoadTrack", name, nodes[0], nodes[2], normals[0], normals[2]);
            auto tR = addPath("RoadTrack", name, nodes[1], nodes[3], normals[1], normals[3]);

            for (auto t : {tL, tR}) {
                t->set("width", toString(trackWidth*0.5));
                way->add("tracks", t->getName());
            }
        }
    }
}

void VRRoadNetwork::computeMarkingsIntersection( VREntityPtr intersection) {
    string name = intersection->getName();

    computeTracksLanes(intersection);

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


void VRRoadNetwork::computeLanes() {
    for (auto road : ontology->getEntities("Road")) computeLanePaths(road);
    for (auto intersection : ontology->getEntities("RoadIntersection")) computeIntersectionLanes(intersection);
}

void VRRoadNetwork::computeSurfaces() {
    for (auto road : ontology->getEntities("Road")) {
        auto roadGeo = createRoadGeometry( road );
        if (!roadGeo) continue;
        roadGeo->hide();
        addChild( roadGeo );
    }

    for (auto intersection : ontology->getEntities("RoadIntersection")) {
        auto iGeo = createIntersectionGeometry( intersection );
        if (!iGeo) continue;
        iGeo->hide();
        addChild( iGeo );
    }
}

void VRRoadNetwork::computeMarkings() {
    for (auto road : ontology->getEntities("Road")) computeMarkingsRoad2( road );
    for (auto intersection : ontology->getEntities("RoadIntersection")) computeMarkingsIntersection( intersection );
}

void VRRoadNetwork::compute() {
    computeIntersections();
    computeLanes();
    computeSurfaces();
    computeMarkings();
}





