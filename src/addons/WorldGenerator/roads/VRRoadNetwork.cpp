#include "VRRoadNetwork.h"
#include "VRRoad.h"
#include "VRRoadIntersection.h"
#include "../terrain/VRTerrain.h"
#include "../VRWorldGenerator.h"
#include "VRAsphalt.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/WorldGenerator/nature/VRNature.h"
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
#include "core/tools/VRAnalyticGeometry.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"

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
    graph = Graph::create();

    //tool = VRPathtool::create();
    asphalt = VRAsphalt::create();
    asphaltArrow = VRAsphalt::create();
    asphaltArrow->setArrowMaterial();

    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );

    collisionMesh = VRGeometry::create("roadsAssetsCollisionShape");
    addChild( collisionMesh );
}

int VRRoadNetwork::getRoadID() { return ++nextRoadID; }
VRAsphaltPtr VRRoadNetwork::getMaterial() { return asphalt; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

vector<Vec3d> VRRoadNetwork::getGraphEdgeDirections(int e) { return graphNormals[e]; }

void VRRoadNetwork::clear() {
	nextRoadID = 0;
	if (world->getOntology()) world->getOntology()->remEntities("Node");
	if (world->getOntology()) world->getOntology()->remEntities("NodeEntry");
	if (world->getOntology()) world->getOntology()->remEntities("Arrow");
	if (world->getOntology()) world->getOntology()->remEntities("GreenBelt");
	if (world->getOntology()) world->getOntology()->remEntities("Lane");
	if (world->getOntology()) world->getOntology()->remEntities("Way");
	if (world->getOntology()) world->getOntology()->remEntities("Road");
	if (world->getOntology()) world->getOntology()->remEntities("RoadMarking");
	if (world->getOntology()) world->getOntology()->remEntities("Path");
	if (world->getOntology()) world->getOntology()->remEntities("RoadTrack");
	if (world->getOntology()) world->getOntology()->remEntities("RoadIntersection");
	if (arrowTexture) arrowTexture = VRTexture::create();
    arrowTemplates.clear();
    arrows->destroy();
    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );

    for (auto road : roads) road->destroy();
    roads.clear();
    for (auto road : ways) road->destroy();
    ways.clear();
    for (auto road : intersections) road->destroy();
    intersections.clear();
    for (auto a : assets) a->destroy();
    assets.clear();
}

VREntityPtr VRRoadNetwork::addNode( Vec3d pos, bool elevate, float elevationOffset ) {
    int nID = graph->addNode();
    graph->setPosition(nID, pose::create(pos));
    return VRRoadBase::addNode(nID, pos, elevate, elevationOffset);
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
            auto dL = marking->get("dashLength");
            float width = w ? toFloat( w->value ) : 0;
            float dashL = dL ? toInt( dL->value ) : 0;
            asphalt->addMarking(rID, toPath(marking, 4), width, dashL);
		}

		for (auto track : tracks) {
            auto w = track->get("width");
            auto dL = track->get("dashLength");
            float width = w ? toFloat( w->value ) : 0;
            float dashL = dL ? toInt( dL->value ) : 0;
            asphalt->addTrack(rID, toPath(track, 4), width, dashL, trackWidth*0.5);
		}

		//cout << "VRRoadNetwork::updateTexture, markings and tracks: " << markings.size() << " " << tracks.size() << endl;
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
	roadEnt->set("name", name);
	for (auto path : paths) roadEnt->add("path", path->getName());
    auto road = VRRoad::create();
    road->setWorld(world);
    road->setEntity(roadEnt);
    addChild(road);
    ways.push_back(road);
	return road;
}

VRRoadPtr VRRoadNetwork::addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3d norm1, Vec3d norm2, int Nlanes ) {
    return addLongRoad(name, type, {node1, node2}, {norm1, norm2}, Nlanes);
}

VRRoadPtr VRRoadNetwork::addLongRoad( string name, string type, vector<VREntityPtr> nodesIn, vector<Vec3d> normalsIn, int Nlanes ) {
    //static VRAnalyticGeometryPtr ana = 0;
    //if (!ana) { ana = VRAnalyticGeometry::create(); addChild(ana); }

    vector<VREntityPtr> nodes;
    vector<Vec3d> norms;

    // check for inflection points!
    for (uint i=1; i<nodesIn.size(); i++) {
        nodes.push_back( nodesIn[i-1] ); norms.push_back( normalsIn[i-1] );
        Vec3d p1 = nodesIn[i-1]->getVec3("position");
        Vec3d p2 = nodesIn[i  ]->getVec3("position");

        path p;
        p.addPoint( pose(p1, normalsIn[i-1]) );
        p.addPoint( pose(p2, normalsIn[i  ]) );
        p.compute(16);

        for (auto t : p.computeInflectionPoints(0, 0, 0.2, 0.1, Vec3i(1,0,1))) { // add inflection points
            auto pnt = p.getPose(t);
            Vec3d n = pnt->dir(); //n.normalize();
            if (terrain) terrain->projectTangent(n, pnt->pos());
            nodes.push_back( addNode(pnt->pos(), true) ); norms.push_back( n );
        }
    }
    nodes.push_back( nodesIn[nodesIn.size()-1] ); norms.push_back( normalsIn[normalsIn.size()-1] );

    if (terrain) {
        for (uint i=0; i<nodesIn.size(); i++) { // project tangents on terrain
            Vec3d p = nodesIn[i]->getVec3("position");
            terrain->projectTangent(normalsIn[i], p);
        }
    }

    /*cout << " -- VRRoadNetwork::addLongRoad nodes ";
    for (int i=0; i<nodes.size(); i++) {
        cout << "  i " << i << nodes[i]->getName() << " n " << norms[i];
    }
    cout << endl;*/

    // add path
    int rID = getRoadID();
    auto path = addPath("Path", name, nodes, norms);
    VRRoadPtr road = addWay(name, { path }, rID, "Road");
    road->setWorld(world);
    road->getEntity()->set("type", type);
    int Nm = Nlanes*0.5;
    for (int i=0; i<Nm; i++) road->addLane(1, 4 );
    for (int i=0; i<Nlanes-Nm; i++) road->addLane(-1, 4 );
    roads.push_back(road);
    roadsByEntity[road->getEntity()] = road;
    return road;
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
        vector<Vec3d> norms;

        for (auto entry : pathEnt->getAllEntities("nodes")) { // TODO: it might be more efficient to have this as outer loop
            Vec3d p = entry->getEntity("node")->getVec3("position");
            Vec3d n = entry->getVec3("direction");
            Vec3d x = Vec3d(0,1,0).cross(n);
            x.normalize();
            float k = width*0.5 + widthSum ;
            Vec3d pi = x*k + p;
            VREntityPtr node = addNode(pi);
            nodes.push_back(node);
            norms.push_back(n*direction);
        }

        if (direction < 0) {
            reverse(nodes.begin(), nodes.end());
            reverse(norms.begin(), norms.end());
        }

        for (uint i=1; i<nodes.size(); i++) {
            connectGraph({nodes[i-1], nodes[i]}, {norms[i-1], norms[i]});
        }

        auto lPath = addPath("Path", "lane", nodes, norms);
		lane->add("path", lPath->getName());
		widthSum += width;
	}
}

void VRRoadNetwork::connectGraph(vector<VREntityPtr> nodes, vector<Vec3d> norms) {
    auto nID1 = nodes[0]->getValue<int>("graphID", -1);
    auto nID2 = nodes[1]->getValue<int>("graphID", -1);
    int eID = graph->connect(nID1, nID2);
    graphNormals[eID] = norms;
}

void VRRoadNetwork::addGuardRail( pathPtr path, float height ) {
    if (!path) return;
	float poleDist = 1.3;
	float poleWidth = 0.2;

	// add poles
	auto pole = VRGeometry::create("pole");
	pole->setPrimitive("Box", "0.02 "+toString(height)+" "+toString(poleWidth)+" 1 1 1");
    pole->setMaterial( world->getMaterial("guardrail") );

	vector<VRGeometryPtr> poles;
	auto addPole = [&](const pose& p) {
	    Vec3d pos = p.pos();
	    Vec3d n = p.dir();
		pos[1] += height*0.5-0.11;
		Vec3d x = -n.cross(Vec3d(0,1,0));
		x.normalize();
		auto po = dynamic_pointer_cast<VRGeometry>( pole->duplicate() );
		po->setPose(x*0.011+pos,n,Vec3d(0,1,0));
		poles.push_back(po);
	};

    float L = path->getLength();
    int N = L / poleDist;
    auto p0 = path->getPose(0);
    auto p1 = path->getPose(1);
    p0->setPos( p0->pos() + p0->dir()*poleWidth );
    p1->setPos( p1->pos() - p1->dir()*poleWidth );
    addPole(*p0); // first pole
    addPole(*p1); // last pole
    for (int i = 0; i< N-1; i++) addPole( *path->getPose( (i+1)*1.0/N ) );

	vector<Vec3d> profile;
    profile.push_back(Vec3d(0.0,height-0.5,0));
    profile.push_back(Vec3d(0.1,height-0.4,0));
    profile.push_back(Vec3d(0.0,height-0.3,0));
    profile.push_back(Vec3d(0.0,height-0.2,0));
    profile.push_back(Vec3d(0.1,height-0.1,0));
    profile.push_back(Vec3d(0.0,height+0.0,0));

	auto rail = VRStroke::create("rail");
	rail->setMaterial( world->getMaterial("guardrail") );
	rail->setPaths({path});
	rail->strokeProfile(profile, false, true, false);
	rail->updateNormals(false);
	//rail->physicalize(true,false,false);
	//rail.showGeometricData("Normals", True);

	//for p in poles: rail.merge(p);
	for (auto p : poles) rail->addChild(p);
	addChild(rail);
	assets.push_back(rail);

	// physics
	auto shape = VRStroke::create("shape");
	shape->setPaths({path});
	shape->strokeProfile({Vec3d(0,0,0), Vec3d(0,height,0)}, false, true, false);
	collisionMesh->merge(shape);
}

void VRRoadNetwork::addKirb( VRPolygonPtr perimeter, float h ) {
    auto path = path::create();
    if (terrain) terrain->elevatePolygon(perimeter);
    Vec3d median = perimeter->getBoundingBox().center();
    perimeter->translate(-median);
    auto points = perimeter->get3();
    int N = points.size();

    for (int i=0; i<N; i++) {
        Vec3d p1 = points[(i-1)%N];
        Vec3d p2 = points[i];
        Vec3d p3 = points[(i+1)%N];
        Vec3d d1 = p2-p1; d1.normalize();
        Vec3d d2 = p3-p2; d2.normalize();
        Vec3d n = d1+d2; n.normalize();
        Vec3d p21 = p2 - d1*0.01;
        Vec3d p23 = p2 + d2*0.01;
        Vec3d p22 = (p21+p23)*0.5;
        path->addPoint( pose(p21, d1) );
        path->addPoint( pose(p22, n ) );
        path->addPoint( pose(p23, d2) );
    }
    path->close();
    path->compute(2);
    auto kirb = VRStroke::create("kirb");
    kirb->addPath(path);

    kirb->strokeProfile({Vec3d(0.0, h, 0), Vec3d(-0.1, h, 0), Vec3d(-0.1, 0, 0)}, 0, 0);
    kirb->updateNormals(1);
    kirb->setMaterial( world->getMaterial("kirb") );

    if (terrain) terrain->elevatePoint(median); // TODO: elevate each point of the polygon
    kirb->translate(median);
    addChild(kirb);
	assets.push_back(kirb);

	// physics
	auto shape = VRStroke::create("shape");
	shape->addPath(path);
	shape->strokeProfile({Vec3d(-0.1, h, 0), Vec3d(-0.1, 0, 0)}, false, true, false);
	collisionMesh->merge(shape);
}

void VRRoadNetwork::physicalizeAssets() {
    collisionMesh->getPhysics()->setDynamic(false);
    collisionMesh->getPhysics()->setShape("Concave");
    collisionMesh->getPhysics()->setPhysicalized(true);
    collisionMesh->setMeshVisibility(false);
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

void VRRoadNetwork::computeSigns() { // add stop lines
    auto assets = world->getAssetManager();
    for (auto signEnt : world->getOntology()->getEntities("Sign")) {
        Vec3d pos = signEnt->getVec3("position");
        Vec3d dir = signEnt->getVec3("direction");
        string type = signEnt->getValue<string>("type", "");
        auto sign = assets->copy(type, pose::create(pos, dir), false);
        if (auto roadEnt = signEnt->getEntity("road")) {
            auto road = roadsByEntity[roadEnt];// get vrroad from roadent
            auto pose = road->getRightEdge(pos);
            auto d = pose->dir(); d[1] = 0; d.normalize();
            pose->setDir(d);
            pose->setUp(Vec3d(0,1,0));
            sign->setPose(pose);

            if (type == "Stop") { // TODO: use lanes widths
                Vec3d x = pose->x();
                Vec3d p1 = pose->pos() - x*3.0;
                Vec3d p2 = pose->pos() - x*0.3;
                auto mL = addPath("StopLine", "Stopline", { road->addNode(0, p1), road->addNode(0, p2) }, { x, x });
                mL->set("width", toString(0.3));
                roadEnt->add("markings", mL->getName());
            }
        }
    }
}

void VRRoadNetwork::computeArrows() {
    for (auto arrow : world->getOntology()->getEntities("Arrow")) {
        float t = toFloat( arrow->get("position")->value );
        auto lane = arrow->getEntity("lane");
        if (!lane || !lane->getEntity("path")) continue;
        auto lpath = toPath( lane->getEntity("path"), 16 );
        t /= lpath->getLength();
        auto dirs = arrow->getAllValues<float>("direction");
        Vec4i drs(999,999,999,999);
        for (uint i=0; i<4 && i < dirs.size(); i++) drs[i] = int(dirs[i]*5/pi)*180/5;
        if (t < 0) t = 1+t; // from the end
        createArrow(drs, min(int(dirs.size()),4), *lpath->getPose(t));
    }
}

void VRRoadNetwork::createArrow(Vec4i dirs, int N, const pose& p) {
    if (N == 0) return;

    //if (arrowTemplates.size() > 20) { cout << "VRRoadNetwork::createArrow, Warning! arrowTexture too big!\n"; return; }

    if (!arrowTemplates.count(dirs)) {
        //cout << "VRRoadNetwork::createArrow " << dirs << "  " << N << endl;
        arrowTemplates[dirs] = arrowTemplates.size();

        VRTextureGenerator tg;
        tg.setSize(Vec3i(400,400,1), true);
        tg.drawFill(Color4f(0,0,1,1));

        for (int i=0; i<N; i++) {
            float a = dirs[i]*pi/180.0;
            Vec3d dir(sin(a), -cos(a), 0);
            Vec2f d02 = Vec2f(0.5,0.5); // rotation point
            Vec3d d03 = Vec3d(0.5,0.5,0); // rotation point

            auto apath = path::create();
            apath->addPoint( pose(Vec3d(0.5,1.0,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
            apath->addPoint( pose(Vec3d(0.5,0.8,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
            apath->addPoint( pose(d03+dir*0.31, dir, Vec3d(0,0,1)) );
            apath->compute(12);
            tg.drawPath(apath, Color4f(1,1,1,1), 0.1);

            auto poly = VRPolygon::create();
            Matrix22<float> R(cos(a), -sin(a), sin(a), cos(a));
            Vec2f A = Vec2f(0.35,0.2)-d02;
            Vec2f B = Vec2f(0.65,0.2)-d02;
            Vec2f C = Vec2f(0.5,0.0)-d02;
            A = R.mult(A); B = R.mult(B); C = R.mult(C);
            poly->addPoint(Vec2d(d02+A));
            poly->addPoint(Vec2d(d02+B));
            poly->addPoint(Vec2d(d02+C));
            tg.drawPolygon(poly, Color4f(1,1,1,1));
        }

        auto aMask = tg.compose(0);
        if (!arrowTexture) arrowTexture = VRTexture::create();
        arrowTexture->merge(aMask, Vec3d(1,0,0));
        asphaltArrow->setTexture(arrowTexture);
        asphaltArrow->setShaderParameter("NArrowTex", (int)arrowTemplates.size());
    }

    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
    Vec4d color = Vec4d((arrowTemplates[dirs]-1)*0.001, 0, 0);
    for (int i=0; i<4; i++) cols->addValue(color);

    VRGeoData gdata;
    gdata.pushQuad(Vec3d(0,0.02,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(2,2), true);
    auto geo = gdata.asGeometry("arrow");
    geo->setPose( pose::create(p) );
    geo->applyTransformation();
    geo->setColors(cols);
    geo->setPositionalTexCoords2D(1.0, 1, Vec2i(0,2));
    addChild(geo);
    arrows->merge(geo);
    geo->destroy();
}

void VRRoadNetwork::computeIntersections() {
    cout << "VRRoadNetwork::computeIntersections\n";
    for (auto node : getRoadNodes() ) {
        auto nodeRoads = getNodeRoads(node);
        if (nodeRoads.size() <= 1) continue; // ignore ends
        auto iEnt = world->getOntology()->addEntity( "intersectionRoad", "RoadIntersection" );
        iEnt->set("ID", toString(getRoadID()));
        auto intersection = VRRoadIntersection::create();
        intersection->setWorld(world);
        intersection->setEntity(iEnt);
        intersections.push_back(intersection);
        addChild(intersection);
        for (auto r : nodeRoads) { intersection->addRoad(r); }
        iEnt->set("node", node->getName());
        intersection->computeLayout(graph);
    }
}

void VRRoadNetwork::computeTracksLanes(VREntityPtr way) {
    auto getBulge = [&](vector<pose>& points, uint i, Vec3d& x) -> float {
        if (points.size() < 2) return 0;
        if (i == 0) return 0;
        if (i == points.size()-1) return 0;

        Vec3d t1 = points[i-1].pos() - points[i].pos();
        t1.normalize();
        Vec3d t2 = points[i+1].pos() - points[i].pos();
        t2.normalize();

        float b1 = -x.dot(t1);
        float b2 = -x.dot(t2);
        return (b1+b2)*trackWidth*0.5;
    };

    for (auto lane : way->getAllEntities("lanes")) {
        if (!lane->is_a("Lane")) continue;
        if (lane->getValue<bool>("pedestrian", false)) continue;
        for (auto pathEnt : lane->getAllEntities("path")) {
            auto path = toPath(pathEnt, 2);
            vector<VREntityPtr> nodes;
            vector<Vec3d> normals;
            auto points = path->getPoints();
            for (uint i=0; i < points.size(); i++) {
                auto& point = points[i];
                Vec3d p = point.pos();
                Vec3d n = point.dir();
                float nL = n.length();
                Vec3d x = n.cross(Vec3d(0,1,0));
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

void VRRoadNetwork::computeLanes() {
    cout << "VRRoadNetwork::computeLanes\n";
    for (auto road : world->getOntology()->getEntities("Road")) computeLanePaths(road);
    for (auto intersection : intersections) {
        intersection->computeLanes(graph);
        intersection->computeTrafficLights();
    }
}

void VRRoadNetwork::computeSurfaces() {
    cout << "VRRoadNetwork::computeSurfaces\n";
    auto computeRoadSurface = [&](VRRoadPtr road) {
        auto roadGeo = road->createGeometry();
        roadGeo->setMaterial( asphalt );
        if (!roadGeo) return;
        roadGeo->getPhysics()->setDynamic(false);
        roadGeo->getPhysics()->setShape("Concave");
        roadGeo->getPhysics()->setPhysicalized(true);
        //addChild( roadGeo );
    };

    for (auto way : ways) computeRoadSurface(way);
    //for (auto road : roads) computeRoadSurface(road);

    for (auto intersection : intersections) {
        auto iGeo = intersection->createGeometry();
        if (!iGeo) continue;
        iGeo->setMaterial( asphalt );
        iGeo->getPhysics()->setDynamic(false);
        iGeo->getPhysics()->setShape("Concave");
        iGeo->getPhysics()->setPhysicalized(true);
        //addChild( iGeo );
    }
}

void VRRoadNetwork::computeMarkings() {
    cout << "VRRoadNetwork::computeMarkings\n";
    for (auto way : world->getOntology()->getEntities("Way")) computeTracksLanes(way);
    for (auto road : ways) road->computeMarkings();
    for (auto intersection : intersections) intersection->computeMarkings();
}

vector<VRPolygonPtr> VRRoadNetwork::computeGreenBelts() {
    cout << "VRRoadNetwork::computeGreenBelts\n";
    vector<VRPolygonPtr> areas;
    for (auto belt : world->getOntology()->getEntities("GreenBelt")) {
        VRPolygonPtr area = VRPolygon::create();
        for (auto pathEnt : belt->getAllEntities("path")) {
            float width = toFloat( belt->get("width")->value ) * 0.3;
            vector<Vec3d> rightPoints;
            for (auto& point : toPath(pathEnt, 16)->getPoses()) {
                Vec3d p = point.pos();
                Vec3d n = point.dir();
                Vec3d x = point.x();
                Vec3d pL = p - x*width;
                Vec3d pR = p + x*width;
                area->addPoint( Vec2d(pL[0],pL[2]) );
                rightPoints.push_back(pR);
            }
            for (auto p = rightPoints.rbegin(); p != rightPoints.rend(); p++) area->addPoint( Vec2d((*p)[0],(*p)[2]) );
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
    computeArrows();
    computeSigns();
    //computeGreenBelts();
    updateAsphaltTexture();
    physicalizeAssets();
}




