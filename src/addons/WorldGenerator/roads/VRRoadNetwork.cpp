#include "VRRoadNetwork.h"
#include "VRRoad.h"
#include "VRRoadIntersection.h"
#include "VRTunnel.h"
#include "VRBridge.h"
#include "../terrain/VRTerrain.h"
#include "../traffic/VRTrafficLights.h"
#include "../VRWorldGenerator.h"
#include "VRTrafficSigns.h"
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
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRSpatialCollisionManager.h"
#endif
#include "core/objects/VRLodTree.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTexture.h"
#ifndef WITHOUT_PANGO_CAIRO
#include "core/tools/VRText.h"
#endif
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include "core/tools/VRAnnotationEngine.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGMatrix22.h>

const double pi = 2*acos(0.0);

using namespace OSG;

VRRoadNetwork::VRRoadNetwork() : VRRoadBase("RoadNetwork") {
    updateCb = VRUpdateCb::create( "roadNetworkUpdate", bind(&VRRoadNetwork::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRRoadNetwork::~VRRoadNetwork() {}

VRRoadNetworkPtr VRRoadNetwork::create() {
    auto rn = VRRoadNetworkPtr( new VRRoadNetwork() );
    rn->init();
    return rn;
}

VRRoadNetworkPtr VRRoadNetwork::ptr() { return dynamic_pointer_cast<VRRoadNetwork>(shared_from_this()); }

void VRRoadNetwork::init() {
    graph = Graph::create();

    //tool = VRPathtool::create();
    asphalt = VRAsphalt::create();
    asphaltArrow = VRAsphalt::create();
    asphaltArrow->setArrowMaterial();

    roadsGeo = VRGeometry::create("roads");
    roadsGeo->hide("SHADOW");
    roadsGeo->setMaterial(asphalt);
    addChild( roadsGeo );

    arrows = VRGeometry::create("arrows");
    arrows->hide("SHADOW");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );

    fences = VRGeometry::create("fences");
    kirbs = VRGeometry::create("kirbs");
    guardRails = VRGeometry::create("guardRails");
    guardRailPoles = VRGeometry::create("guardRailPoles");
    addChild( fences );
    addChild( kirbs );
    addChild( guardRails );
    addChild( guardRailPoles );

    auto baseMaterialPole = VRMaterial::get("trafficSignalsMatPole");
    baseMaterialPole->setDiffuse(Color3f(0.3,0.3,0.3));
    baseMaterialPole->setLit(true);

    auto baseMaterialTop = VRMaterial::get("trafficSignalsMatTop");
    baseMaterialTop->setDiffuse(Color3f(0.1,0.1,0.1));
    baseMaterialTop->setLit(true);

    trafficSignalsGeo = VRGeometry::create("trafficSignalsGeo");
    trafficSignalsGeo->setMaterial(baseMaterialTop);
    trafficSignalsPolesGeo = VRGeometry::create("trafficSignalsPolesGeo");
    trafficSignalsPolesGeo->setMaterial(baseMaterialPole);
    addChild(trafficSignalsGeo);
    addChild(trafficSignalsPolesGeo);

    collisionMesh = VRGeometry::create("roadsAssetsCollisionShape");
    collisionMesh->hide("SHADOW");
    addChild( collisionMesh );
}

int VRRoadNetwork::getRoadID() { return ++nextRoadID; }
VRAsphaltPtr VRRoadNetwork::getMaterial() { return asphalt; }
VRAsphaltPtr VRRoadNetwork::getArrowMaterial() { return asphaltArrow; }
GraphPtr VRRoadNetwork::getGraph() { return graph; }

vector<Vec3d> VRRoadNetwork::getGraphEdgeDirections(int e) { return graphNormals[e]; }

void VRRoadNetwork::clear() {
    auto o = ontology.lock();
	nextRoadID = 0;
	if (o) o->remEntities("Node");
	if (o) o->remEntities("NodeEntry");
	if (o) o->remEntities("Arrow");
	if (o) o->remEntities("GreenBelt");
	if (o) o->remEntities("Lane");
	if (o) o->remEntities("Way");
	if (o) o->remEntities("Road");
	if (o) o->remEntities("RoadMarking");
	if (o) o->remEntities("Path");
	if (o) o->remEntities("RoadTrack");
	if (o) o->remEntities("RoadIntersection");
	if (arrowTexture) arrowTexture = VRTexture::create();
    arrowTemplates.clear();
    arrows->destroy();
    arrows = VRGeometry::create("arrows");
    arrows->setMaterial(asphaltArrow);
    addChild( arrows );

    auto w = world.lock();
    guardRailPoles->destroy();
    guardRailPoles = VRGeometry::create("guardRailPoles");
    guardRailPoles->setMaterial( w->getMaterial("guardrail") );
    addChild( guardRailPoles );

    for (auto road : roads) road->destroy();
    roads.clear();
    for (auto road : ways) road->destroy();
    ways.clear();
    for (auto road : intersections) road->destroy();
    intersections.clear();
    for (auto a : assets) a->destroy();
    assets.clear();
}

void VRRoadNetwork::updateAsphaltTexture() {
    auto o = ontology.lock();
	asphalt->clearTexture();
	for (auto road : o->getEntities("Way")) {
        auto id = road->get("ID");
		if (!id) continue;
		int rID = toInt( id->value );

		auto markings = road->getAllEntities("markings");
		auto tracks = road->getAllEntities("tracks");

		for (auto marking : markings) {
            auto w = marking->get("width");
            auto dL = marking->get("dashLength");
            string c = marking->getValue<string>("color", "white");
            int colorID = 1;
            if (c == "yellow") colorID = 2;
            float width = w ? toFloat( w->value ) : 0;
            float dashL = dL ? toFloat( dL->value ) : 0;
            asphalt->addMarking(rID, toPath(marking, 4), width, dashL, 0, colorID);
		}

		for (auto track : tracks) {
            auto w = track->get("width");
            auto dL = track->get("dashLength");
            float width = w ? toFloat( w->value ) : 0;
            float dashL = dL ? toFloat( dL->value ) : 0;
            asphalt->addTrack(rID, toPath(track, 4), width, dashL, trackWidth*0.5);
		}

		//cout << "VRRoadNetwork::updateTexture, markings and tracks: " << markings.size() << " " << tracks.size() << endl;
	}
	asphalt->updateTexture();
}

VREntityPtr VRRoadNetwork::addGreenBelt( VREntityPtr road, float width ) {
    auto o = ontology.lock();
	auto g = o->addEntity( road->getName()+"GreenBelt", "GreenBelt");
	g->set("width", toString(width));
	road->add("lanes", g->getName());
	return g;
}

VRRoadPtr VRRoadNetwork::addWay( string name, vector<VREntityPtr> paths, int rID, string type ) {
    auto o = ontology.lock();
	auto roadEnt = o->addEntity( name+"Road", type );
	roadEnt->set("ID", toString(rID));
	roadEnt->set("name", name);
	for (auto path : paths) roadEnt->add("path", path->getName());
    auto road = VRRoad::create();
    road->setWorld(world.lock());
    road->setEntity(roadEnt);
    ways.push_back(road);
	return road;
}

VRRoadPtr VRRoadNetwork::addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3d norm1, Vec3d norm2, int Nlanes ) {
    return addLongRoad(name, type, {node1, node2}, {norm1, norm2}, Nlanes);
}

VRRoadPtr VRRoadNetwork::addLongRoad( string name, string type, vector<VREntityPtr> nodesIn, vector<Vec3d> normalsIn, int Nlanes ) {
    if (nodesIn.size() != normalsIn.size()) {
        cout << "Warning in VRRoadNetwork::addLongRoad: ignore road '" << name << "', nodes and normals vector sizes mismatch!" << endl;
        return 0;
    }

    if (nodesIn.size() <= 1) {
        cout << "Warning in VRRoadNetwork::addLongRoad: ignore road '" << name << "', not enough nodes: " << nodesIn.size() << endl;
        return 0;
    }

    vector<VREntityPtr> nodes;
    vector<Vec3d> norms;

    // check for inflection points!
    for (unsigned int i=1; i<nodesIn.size(); i++) {
        nodes.push_back( nodesIn[i-1] );
        norms.push_back( normalsIn[i-1] );
        Vec3d p1 = nodesIn[i-1]->getVec3("position");
        Vec3d p2 = nodesIn[i  ]->getVec3("position");

        Path p;
        p.addPoint( Pose(p1, normalsIn[i-1]) );
        p.addPoint( Pose(p2, normalsIn[i  ]) );
        p.compute(16);

        for (auto t : p.computeInflectionPoints(0, 0, 0.2, 0.1, Vec3i(1,0,1))) { // add inflection points
            auto pnt = p.getPose(t);
            Vec3d n = pnt->dir(); //n.normalize();
            if (auto t = terrain.lock()) t->projectTangent(n, pnt->pos());
            nodes.push_back( addNode(pnt->pos(), true) ); norms.push_back( n );
        }
    }
    nodes.push_back( nodesIn[nodesIn.size()-1] );
    norms.push_back( normalsIn[normalsIn.size()-1] );

    if (auto t = terrain.lock()) {
        for (unsigned int i=0; i<nodesIn.size(); i++) { // project tangents on terrain
            Vec3d p = nodesIn[i]->getVec3("position");
            t->projectTangent(normalsIn[i], p);
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
    road->getEntity()->set("type", type);
    int Nm = Nlanes*0.5;
    for (int i=0; i<Nm; i++) road->addLane(1, 4 );
    for (int i=0; i<Nlanes-Nm; i++) road->addLane(-1, 4 );
    roads.push_back(road);
    roadsByEntity[road->getEntity()] = road;
    string streetName = road->getEntity()->getValue<string>("name", "unnamed");
    roadsByName[streetName].push_back(road);
    return road;
}

void VRRoadNetwork::computeLanePaths( VREntityPtr road ) {
    if (road->getAllEntities("path").size() > 1) cout << "Warning in VRRoadNetwork::computeLanePaths, road " << road->getName() << " has more than one path!\n";

    auto pathEnt = road->getEntity("path");
    if (!pathEnt) return;
	auto lanes = road->getAllEntities("lanes");
    vector<vector<int>> lanesD1;
    vector<vector<int>> lanesD2;

	float roadWidth = 0;
	for (auto lane : lanes) roadWidth += toFloat( lane->get("width")->value );

	float widthSum = -roadWidth*0.5;
	for (unsigned int li=0; li<lanes.size(); li++) {
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

        vector<int> laneEdges;
        for (unsigned int i=1; i<nodes.size(); i++) {
            connectGraph({nodes[i-1], nodes[i]}, {norms[i-1], norms[i]}, lane);
            int nID1 = nodes[i-1]->getValue<int>("graphID", -1);
            int nID2 = nodes[i]->getValue<int>("graphID", -1);
            int eID = graph->getEdgeID(nID1,nID2);
            laneEdges.push_back(eID);
            //cout << toString(laneEdges[i-1]) << endl;
        }

        auto lPath = addPath("Path", "lane", nodes, norms);
		lane->add("path", lPath->getName());
		widthSum += width;
		if (direction > 0) { lanesD1.push_back(laneEdges); }
		if (direction < 0) { lanesD2.push_back(laneEdges); }
    }

	if (lanesD1.size()>1) {
        for (unsigned int i = 0; i<lanesD1[0].size();i++) {
            for (unsigned int j = 1; j<lanesD1.size();j++) {
                ///checking minimum length for lane relations
                if (graph->getEdgeLength(lanesD1[j][i]) < 10) continue;
                if (graph->getEdgeLength(lanesD1[j-1][i]) < 10) continue;
                graph->addRelation(lanesD1[j][i],lanesD1[j-1][i]);
                //cout << toString(lanesD1[j][i]) << " -- " << toString(lanesD1[j-1][i]) << endl;
            }
        }
	}
    if (lanesD2.size()>1) {
        for (unsigned int i = 0; i<lanesD2[0].size();i++) {
            for (unsigned int j = 1; j<lanesD2.size();j++) {
                ///checking minimum length for lane relations
                if (graph->getEdgeLength(lanesD2[j][i]) < 10) continue;
                if (graph->getEdgeLength(lanesD2[j-1][i]) < 10) continue;
                graph->addRelation(lanesD2[j][i],lanesD2[j-1][i]);
                //cout << toString(lanesD2[j][i]) << " -- " << toString(lanesD2[j-1][i]) << endl;
            }
        }
    }
}

void VRRoadNetwork::addFence( PathPtr path, float height ) {
    auto w = world.lock();
    if (!path) return;

	vector<Vec3d> profile;
    profile.push_back(Vec3d(0,0,0));
    profile.push_back(Vec3d(0,height,0));

	auto fence = VRStroke::create("fence");
	fence->setPaths({path});
	fence->strokeProfile(profile, false, true, false);
	fences->merge(fence);

	// physics
#ifndef WITHOUT_BULLET
	auto shape = VRStroke::create("shape");
	shape->setPaths({path});
	shape->strokeProfile({Vec3d(0,0,0), Vec3d(0,height,0)}, false, true, false);
	if (auto w = world.lock()) w->getPhysicsSystem()->add(shape, fences->getID());
#endif
}

void VRRoadNetwork::addGuardRail( PathPtr path, float height ) {
    auto w = world.lock();
    if (!path) return;
	float poleDist = 1.3;
	float poleWidth = 0.2;

	vector<PosePtr> poles;
	auto addPole = [&](Pose& p) {
	    Vec3d pos = p.pos();
	    Vec3d n = p.dir();
		pos[1] += height*0.5-0.11;
		Vec3d x = -n.cross(Vec3d(0,1,0));
		x.normalize();
		auto po = Pose::create(x*0.011+pos,n,Vec3d(0,1,0));
		poles.push_back(po);
	};

    auto p0 = path->getPose(0);
    auto p1 = path->getPose(1);
    p0->setPos( p0->pos() + p0->dir()*poleWidth );
    p1->setPos( p1->pos() - p1->dir()*poleWidth );
    addPole(*p0); // first pole
    addPole(*p1); // last pole

    for (int j=0; j<path->size()-1; j++) {
        float L = path->getLength(j, j+1);
        int N = L / poleDist;
        int N2 = N;
        if (j == path->size()-2) N2--;
        for (int i = 0; i<N2; i++) addPole( *path->getPose( (i+1)*1.0/N, j, j+1 ) );
    }

	vector<Vec3d> profile;
    profile.push_back(Vec3d(0.0,height-0.5,0));
    profile.push_back(Vec3d(0.1,height-0.4,0));
    profile.push_back(Vec3d(0.0,height-0.3,0));
    profile.push_back(Vec3d(0.0,height-0.2,0));
    profile.push_back(Vec3d(0.1,height-0.1,0));
    profile.push_back(Vec3d(0.0,height+0.0,0));

	auto rail = VRStroke::create("rail");
	rail->setPaths({path});
	rail->strokeProfile(profile, false, true, false);
	guardRails->merge(rail);
	//rail->physicalize(true,false,false);
	//rail.showGeometricData("Normals", True);

	// add poles
	auto pole = VRGeometry::create("pole");
	pole->setPrimitive("Box 0.02 "+toString(height)+" "+toString(poleWidth)+" 1 1 1");
	for (auto p : poles) guardRailPoles->merge(pole, p);
    guardRailPoles->setMaterial( w->getMaterial("guardrail") );

	// physics
#ifndef WITHOUT_BULLET
	auto shape = VRStroke::create("shape");
	shape->setPaths({path});
	shape->strokeProfile({Vec3d(0,0,0), Vec3d(0,height,0)}, false, true, false);
	if (auto w = world.lock()) w->getPhysicsSystem()->add(shape, guardRails->getID());
#endif
}

void VRRoadNetwork::addKirb( VRPolygonPtr perimeter, float h ) {
    auto w = world.lock();
    auto path = Path::create();
    if (auto t = terrain.lock()) t->elevatePolygon(perimeter);
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
        Vec3d p21 = p2 - d1*0.01 + median;
        Vec3d p23 = p2 + d2*0.01 + median;
        Vec3d p22 = (p21+p23)*0.5;
        if (auto t = terrain.lock()) {
            p21 = t->elevatePoint(p21);
            p22 = t->elevatePoint(p22);
            p23 = t->elevatePoint(p23);
        }
        path->addPoint( Pose(p21, d1) );
        path->addPoint( Pose(p22, n ) );
        path->addPoint( Pose(p23, d2) );
    }
    path->close();
    path->compute(2);

    auto kirb = VRStroke::create("kirb");
    kirb->addPath(path);
    kirb->strokeProfile({Vec3d(0.0, h, 0), Vec3d(-0.1, h, 0), Vec3d(-0.1, 0, 0)}, 0, 1, 0);
    kirbs->merge(kirb);

	// physics
#ifndef WITHOUT_BULLET
	auto shape = VRStroke::create("shape");
	shape->addPath(path);
	shape->strokeProfile({Vec3d(-0.1, h, 0), Vec3d(-0.1, 0, 0)}, false, true, false);
	if (auto w = world.lock()) w->getPhysicsSystem()->add(shape, kirbs->getID());
#endif
}

void VRRoadNetwork::physicalizeAssets(Boundingbox volume) {
    /*collisionMesh->getPhysics()->setDynamic(false);
    collisionMesh->getPhysics()->setShape("Concave");
    collisionMesh->getPhysics()->setPhysicalized(true);
    collisionMesh->setMeshVisibility(false);*/
}

vector<VREntityPtr> VRRoadNetwork::getRoadNodes() { // all nodes from all paths from all roads
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

void replaceChar(string& txt, char c1, char c2) {
    for (int i=0; i<int(txt.size()); i++)
        if (txt[i] == c1) txt[i] = c2;
}

void VRRoadNetwork::computeSigns() {
    auto w = world.lock();
    auto assets = w->getAssetManager();
    for (auto signEnt : w->getOntology()->getEntities("Sign")) {
        Vec3d pos = signEnt->getVec3("position");
        Vec3d dir = signEnt->getVec3("direction");
        string type = signEnt->getValue<string>("type", "");
        bool osmSign = (type == "OSMSign");
        if (signEnt->is_a("TrafficLight")) continue;
        //cout << " sign: " << type << endl;
        auto sign = assets->copy(type, Pose::create(pos, dir), false);
        if (!sign && !osmSign) {
            sign = assets->copy("Sign", Pose::create(pos, dir), false);
            if (sign) { // TODO: add label
                auto surface = dynamic_pointer_cast<VRGeometry>( sign->findAll("Sign")[3] );
                surface->makeUnique();
                replaceChar(type, ' ', '\n');
                auto m = VRMaterial::create("sign");
#ifndef WITHOUT_PANGO_CAIRO
                auto tex = VRText::get()->create(type, "Mono.ttf", 20, 3, Color4f(0,0,0,1), Color4f(1,1,1,1));
                m->setTexture(tex);
#endif
                surface->setMaterial(m);
            }
        }

        if (sign) {
            lodTree->addObject(sign, sign->getWorldPosition(), 3, false);
            sign->setDir(dir);
        }

        if ( osmSign ) {
            auto tfsigns = w->getTrafficSigns();
            auto input = signEnt->getValue<string>("info", "");
            //cout << "--------------OSM sign " << input << endl;
            if (auto laneEnt = signEnt->getEntity("lanes")) {
                auto roadEnt = laneEnt->getEntity("road");
                auto road = roadsByEntity[roadEnt];// get vrroad from roadent
                auto pose = road->getRightEdge(pos);
                auto d = pose->dir(); d[1] = 0; d.normalize();
                if (laneEnt->get("direction")->value == "-1" || input == "CN:Prohibitory:5") { pose = road->getLeftEdge(pos); d=-d; }
                if (input == "CN:Indicative:7") { pose = road->getSplit(pos); pose->setPos(pose->pos() - Vec3d(0,1.5,0)); }

                pose->setDir(d);
                pose->setUp(Vec3d(0,1,0));
                tfsigns->addSign(input, pose);

                if (input == "CN:Prohibitory:1") {
                    Vec3d x = pose->x();
                    auto width = road->getWidth();
                    Vec3d p1 = pose->pos() - x*0.5;
                    Vec3d p2 = pose->pos() - x*width + x*0.5;
                    auto mL = road->addPath("StopLine", "Stopline", { road->addNode(0, p2), road->addNode(0, p1) }, { x, x });
                    mL->set("width", toString(0.3));
                    mL->set("color", "yellow");
                    roadEnt->add("markings", mL->getName());
                }
            }
        }

        if (!sign) continue;
        if (auto laneEnt = signEnt->getEntity("lanes")) {
            auto roadEnt = laneEnt->getEntity("road");
            auto road = roadsByEntity[roadEnt];// get vrroad from roadent
            auto pose = road->getRightEdge(pos);
            auto d = pose->dir(); d[1] = 0; d.normalize();
            if (laneEnt->get("direction")->value == "-1") { pose = road->getLeftEdge(pos); d=-d; }
            pose->setDir(-d); // sign looks against road direction
            pose->setUp(Vec3d(0,1,0));
            sign->setPose(pose);

            if (type == "Stop") { // TODO: use lanes widths
                Vec3d x = pose->x();
                Vec3d p1 = pose->pos() + x*0.5;
                Vec3d p2 = pose->pos() + x*2.8;
                auto mL = road->addPath("StopLine", "Stopline", { road->addNode(0, p1), road->addNode(0, p2) }, { x, x });
                mL->set("width", toString(0.3));
                mL->set("color", "yellow");
                roadEnt->add("markings", mL->getName());
            }
        }
#ifndef WITHOUT_BULLET
        if (auto w = world.lock()) w->getPhysicsSystem()->addQuad(0.15, 2, *sign->getPose(), sign->getID());
#endif
    }
}

void VRRoadNetwork::setRoadStyle(int aType) { arrowType = aType; }
int VRRoadNetwork::getArrowStyle() { return arrowType; }

void VRRoadNetwork::computeArrows() {
    arrowTemplates.clear();
    arrowTexture = VRTexture::create();

    auto w = world.lock();
    for (auto arrow : w->getOntology()->getEntities("Arrow")) {
        float t = toFloat( arrow->get("position")->value );
        auto lane = arrow->getEntity("lane");
        if (!lane || !lane->getEntity("path")) continue;
        auto lpath = toPath( lane->getEntity("path"), 16 );
        t /= lpath->getLength();

        auto dirs = arrow->getAllValues<float>("direction");
        map<int, int> uniqueDirs;
        for (auto& d : dirs) uniqueDirs[ int(d*5/pi)*180/5 ] = 0; // discretize directions
        Vec4i drs(999,999,999,999);
        int N = min(int(uniqueDirs.size()),4);
        int i=0;
        for (auto d : uniqueDirs) {
            drs[i] = d.first;
            i++;
            if (i >= N) break;
        }

        if (t < 0) t = 1+t; // from the end
        auto pose = lpath->getPose(t);
        auto dir = pose->dir();
        if ( arrow->get("offset") ) {
            float offset = toFloat(arrow->get("offset")->value);
            pose->setPos(pose->pos() + dir*offset);
        }
        createArrow(drs, N, *pose, arrow->getValue<int>("type", 0));
    }
}

void VRRoadNetwork::createArrow(Vec4i dirs, int N, const Pose& p, int type) {
    if (N == 0) return;

    if (!arrowTemplates.count(dirs)) {
        int aID = arrowTemplates.size();
        arrowTemplates[dirs] = aID;
        //cout << "VRRoadNetwork::createArrow " << dirs << "  " << N << ", ID: " << arrowTemplates[dirs] << endl;

        VRTextureGenerator tg;
        tg.setSize(Vec3i(400,400,1), false);
        tg.drawFill(Color4f(0,0,1,1));

        for (int i=0; i<N; i++) {
            float a = dirs[i]*pi/180.0;
            Vec3d dir(sin(a), -cos(a), 0);
            Vec2f d02 = Vec2f(0.5,0.5); // rotation point
            Vec3d d03 = Vec3d(0.5,0.5,0); // rotation point

            if (type == 0) {
                auto apath = Path::create();
                apath->addPoint( Pose(Vec3d(0.5,1.0,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
                apath->addPoint( Pose(Vec3d(0.5,0.8,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
                apath->addPoint( Pose(d03+dir*0.31, dir, Vec3d(0,0,1)) );
                apath->compute(12);
                tg.drawPath(apath, Color4f(1,1,1,1), 0.1);
            }

            if (type == 1) {
                auto apath = Path::create();
                apath->addPoint( Pose(Vec3d(0.5,1.0,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
                apath->addPoint( Pose(Vec3d(0.5,0.5,0), Vec3d(0,-1,0), Vec3d(0,0,1)) );
                apath->compute(2);
                tg.drawPath(apath, Color4f(1,1,1,1), 0.1);

                apath = Path::create();
                apath->addPoint( Pose(Vec3d(0.5,0.5,0), dir, Vec3d(0,0,1)) );
                apath->addPoint( Pose(d03+dir*0.31, dir, Vec3d(0,0,1)) );
                apath->compute(2);
                float w = 0.15;
                if (a == 0) w = 0.1;
                tg.drawPath(apath, Color4f(1,1,1,1), w);
            }

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

    Color4f color(arrowTemplates[dirs]*0.001, 0, 0, 0);

    VRGeoData gdata;
    gdata.pushQuad(Vec3d(0,0.025,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(2,2), true);
    gdata.addVertexColors(color);
    auto geo = gdata.asGeometry("arrow");
    geo->setPositionalTexCoords2D(1.0, 1, Vec2i(0,2));
    arrows->merge(geo, Pose::create(p));
}

VREntityPtr VRRoadNetwork::getLane(int eID) { return graphEdgeEntities[eID]; }
int VRRoadNetwork::getLaneID(VREntityPtr lane) { return graphEdgeIDs[lane]; }

vector<VRRoadPtr> VRRoadNetwork::getNodeRoads(VREntityPtr node) {
    vector<VREntityPtr> nPaths;
    for (auto nE : node->getAllEntities("paths")) {
        auto path = nE->getEntity("path");
        nPaths.push_back( path );
    }

    vector<VRRoadPtr> res; //= ontology->process("q(r):Node("+node->getName()+");Road(r);has(r.path.nodes,"+node->getName()+")");
    for (auto road : roads) {
        bool added = false;
        for (auto rpath : road->getEntity()->getAllEntities("path")) {
            for (auto npath : nPaths) {
                if (rpath == npath) { res.push_back(road); added = true; break; }
            }
            if (added) break;
        }
    }
    return res;
}

void VRRoadNetwork::computeIntersections() {
    auto w = world.lock();
    cout << "VRRoadNetwork::computeIntersections\n";
    vector<VRRoadPtr> newRoads;
    for (auto road : roads) {
        for (auto r : road->splitAtIntersections(ptr())) newRoads.push_back(r);
    }

    for (auto r : newRoads) {
        roads.push_back(r);
        ways.push_back(r);
        roadsByEntity[r->getEntity()] = r;
        string streetName = r->getEntity()->getValue<string>("name", "unnamed");
        roadsByName[streetName].push_back(r);
    }

    for (auto node : getRoadNodes()) {
        auto nodeRoads = getNodeRoads(node);
        if (nodeRoads.size() <= 1) continue; // ignore ends
        auto iEnt = w->getOntology()->addEntity( "intersectionRoad", "RoadIntersection" );
        iEnt->set("ID", toString(getRoadID()));
        auto intersection = VRRoadIntersection::create();
        intersection->setWorld(w);
        intersection->setEntity(iEnt);
        intersections.push_back(intersection);
        intersectionsByEntity[iEnt] = intersection;
        addChild(intersection); // TODO: remove when managed stuff appended to intersection node

        for (auto r : nodeRoads) { intersection->addRoad(r); }
        iEnt->set("node", node->getName());
        intersection->computeLayout(graph);
    }
}

VREntityPtr VRRoadNetwork::getLaneSegment(int edgeID){
    if (!laneSegmentsByEdgeID.count(edgeID) || edgeID < 0) return 0;
    return laneSegmentsByEdgeID[edgeID];
}

VRTunnelPtr VRRoadNetwork::addTunnel(VRRoadPtr road) { auto t = VRTunnel::create(road); addChild(t); tunnels.push_back(t); return t; }
VRBridgePtr VRRoadNetwork::addBridge(VRRoadPtr road) { auto b = VRBridge::create(road); addChild(b); bridges.push_back(b); return b; }

void VRRoadNetwork::computeTracksLanes(VREntityPtr way) {
    auto getBulge = [&](vector<Pose>& points, unsigned int i, Vec3d& x) -> float {
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
        if (lane->is_a("ParkingLane")) continue;
        if (lane->getValue<bool>("pedestrian", false)) continue;
        for (auto pathEnt : lane->getAllEntities("path")) {
            auto path = toPath(pathEnt, 2);
            vector<VREntityPtr> nodes;
            vector<Vec3d> normals;
            auto points = path->getPoints();
            for (unsigned int i=0; i < points.size(); i++) {
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
    auto w = world.lock();
    for (auto road : w->getOntology()->getEntities("Road")) computeLanePaths(road);
    for (auto intersection : intersections) {
        intersection->computeLanes(graph);
        intersection->computeTrafficLights();
    }
}

void VRRoadNetwork::computeSurfaces() {
    cout << "VRRoadNetwork::computeSurfaces\n";
    auto computeRoadSurface = [&](VRRoadPtr road) {
        auto roadGeo = road->createGeometry();
        if (!roadGeo) return;
        roadsGeo->merge(roadGeo);
#ifndef WITHOUT_BULLET
        if (auto w = world.lock()) w->getPhysicsSystem()->add(roadGeo, roadGeo->getID());
#endif
    };

    for (auto way : ways) computeRoadSurface(way);

    for (auto intersection : intersections) {
        auto iGeo = intersection->createGeometry();
        if (!iGeo) continue;
        //iGeo->setMaterial( asphalt );
        roadsGeo->merge(iGeo);
#ifndef WITHOUT_BULLET
        if (auto w = world.lock()) w->getPhysicsSystem()->add(iGeo, iGeo->getID());
#endif
    }

    auto stoneMat = VRMaterial::create("stone");
    stoneMat->setDiffuse(Color3f(0.7,0.7,0.7));

    for (auto tunnel : tunnels) {
        tunnel->createGeometry();
        dynamic_pointer_cast<VRGeometry>(tunnel->getChild(0))->setMaterial(stoneMat);
    }

    for (auto bridge : bridges) {
        bridge->createGeometry();
        dynamic_pointer_cast<VRGeometry>(bridge->getChild(0))->setMaterial(stoneMat);
    }
}

void VRRoadNetwork::computeIntersectionSemantic() {
    cout << "VRRoadNetwork::computeIntersectionSemantic\n";
    auto w = world.lock();
    for (auto intersec : intersections) {
        intersec->computeSemantics();
    }
    //cout << "VRRoadNetwork::computeIntersectionSemantic - Intersections: " << intersections.size() << endl;
}

void VRRoadNetwork::computeMarkings() {
    cout << "VRRoadNetwork::computeMarkings\n";
    auto w = world.lock();
    for (auto way : w->getOntology()->getEntities("Way")) computeTracksLanes(way);
    for (auto road : ways) road->computeMarkings();
    for (auto intersection : intersections) intersection->computeMarkings();
}

vector<VRRoadPtr> VRRoadNetwork::getRoads() { return roads; }
vector<VRRoadIntersectionPtr> VRRoadNetwork::getIntersections() { return intersections; }

vector<VRPolygonPtr> VRRoadNetwork::computeGreenBelts() {
    cout << "VRRoadNetwork::computeGreenBelts\n";
    vector<VRPolygonPtr> areas;
    auto w = world.lock();
    for (auto belt : w->getOntology()->getEntities("GreenBelt")) {
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
    computeIntersectionSemantic();
    //computeGreenBelts();
    updateAsphaltTexture();
    //physicalizeAssets();
    collisionMesh->setMeshVisibility(false);

    auto w = world.lock();
    if (w) {
        guardRails->setMaterial( w->getMaterial("guardrail") );
        fences->setMaterial( w->getMaterial("fence") );
        kirbs->setMaterial( w->getMaterial("kirb") );
    }

	guardRails->updateNormals(0);
    kirbs->updateNormals(1);
	fences->updateNormals(0);
}

VRGeometryPtr VRRoadNetwork::getAssetCollisionObject() { return collisionMesh; }

VRRoadPtr VRRoadNetwork::getRoad(VREntityPtr road) {
    if (roadsByEntity.count(road)) return roadsByEntity[road];
    return 0;
}

vector<VRRoadPtr> VRRoadNetwork::getRoadByName(string name) {
    if (roadsByName.count(name)) return roadsByName[name];
    return vector<VRRoadPtr>();
}

VRRoadIntersectionPtr VRRoadNetwork::getIntersection(VREntityPtr intersection) {
    if (intersectionsByEntity.count(intersection)) return intersectionsByEntity[intersection];
    return 0;
}

VRGeometryPtr VRRoadNetwork::getTrafficSignalsGeo() { return trafficSignalsGeo; }
VRGeometryPtr VRRoadNetwork::getTrafficSignalsPolesGeo() { return trafficSignalsPolesGeo; }

vector<VREntityPtr> VRRoadNetwork::getPreviousRoads(VREntityPtr road) {
	auto getPreviousPaths = [](VREntityPtr path) {
		auto nes = path->getAllEntities("nodes");
		auto ne = nes[0];
		auto n = ne->getEntity("node");
		auto paths = n->getAllEntities("paths");
		vector<VREntityPtr> res;
		for (auto e : paths) {
            if (e->getName() == ne->getName()) continue;
            res.push_back(e->getEntity("path"));
		}
		return res;
	};

	auto getAdjascendRoads = [](VREntityPtr road) {
		vector<VREntityPtr> res;
		for (auto i : road->getAllEntities("intersections")) {
            for (auto r : i->getAllEntities("roads")) {
                if (r->getName() == road->getName()) continue;
                res.push_back(r);
            }
		}
		return res;
	};

    auto path = road->getEntity("path");
    vector<VREntityPtr> res;
    for (auto aroad : getAdjascendRoads(road)) {
        auto rpName = aroad->getEntity("path")->getName();
        for (auto p1 : getPreviousPaths(path)) {
            if (p1->getName() == rpName) res.push_back(aroad);
            for (auto p2 : getPreviousPaths(p1)) {
                if (p2->getName() == rpName) res.push_back(aroad);
            }
        }
    }
    return res;
}

vector<VREntityPtr> VRRoadNetwork::getNextRoads(VREntityPtr road) {
	auto getNextPaths = [](VREntityPtr path) {
		auto nes = path->getAllEntities("nodes");
		auto ne = nes[nes.size()-1];
		auto n = ne->getEntity("node");
		auto paths = n->getAllEntities("paths");
		vector<VREntityPtr> res;
		for (auto e : paths) {
            if (e->getName() == ne->getName()) continue;
            res.push_back(e->getEntity("path"));
		}
		return res;
	};

	auto getAdjascendRoads = [](VREntityPtr road) {
		vector<VREntityPtr> res;
		for (auto i : road->getAllEntities("intersections")) {
            for (auto r : i->getAllEntities("roads")) {
                if (r->getName() == road->getName()) continue;
                res.push_back(r);
            }
		}
		return res;
	};

    auto path = road->getEntity("path");
    vector<VREntityPtr> res;
    for (auto aroad : getAdjascendRoads(road)) {
        auto rpName = aroad->getEntity("path")->getName();
        for (auto p1 : getNextPaths(path)) {
            if (p1->getName() == rpName) res.push_back(aroad);
            for (auto p2 : getNextPaths(p1)) {
                if (p2->getName() == rpName) res.push_back(aroad);
            }
        }
    }
    return res;
}

VREntityPtr VRRoadNetwork::addNode( Vec3d pos, bool elevate, float elevationOffset ) {
    int nID = graph->addNode();
    graph->setPosition(nID, Pose::create(pos));
    return VRRoadBase::addNode(nID, pos, elevate, elevationOffset);
}

void VRRoadNetwork::connectGraph(vector<VREntityPtr> nodes, vector<Vec3d> norms, VREntityPtr lane) {
    auto nID1 = nodes[0]->getValue<int>("graphID", -1);
    auto nID2 = nodes[1]->getValue<int>("graphID", -1);
    int eID = graph->connect(nID1, nID2);
    graphNormals[eID] = norms;
    graphEdgeEntities[eID] = lane;
    graphEdgeIDs[lane] = eID;

    if (eID < 0) return;
    auto w = world.lock();
    auto laneSegment = w->getOntology()->addEntity("laneSegment", "LaneSegment");
    laneSegment->set("graphID", toString(eID));
    laneSegmentsByEdgeID[eID] = laneSegment;
}


PosePtr VRRoadNetwork::getPosition(Graph::position p) {
    int eID = p.edge;
    if (eID < 0) return 0;
    auto edge = graph->getEdge(eID);
    if (edge.ID < 0) return 0;

    int n1 = edge.from;
    int n2 = edge.to;

    if (!graphNormals.count(eID)) return 0;
    auto eNorms = graphNormals[eID];
    if (!graphEdgeEntities.count(eID)) return 0;
    auto lane = graphEdgeEntities[eID];
    auto pathE = lane->getEntity("path");
    auto nodes = pathE->getAllEntities("nodes");

    Vec3d node1;
    Vec3d node2;

    for (auto nE : nodes) {
        auto n = nE->getEntity("node");
        if (!n) continue;
        if (n->getValue<int>("graphID", 0) == n1) node1 = n->getVec3("position");
        if (n->getValue<int>("graphID", 0) == n2) node2 = n->getVec3("position");
    }

    Path path;
    path.addPoint( Pose(node1, eNorms[0]) );
    path.addPoint( Pose(node2, eNorms[1]) );
    return path.getPose(p.pos, 0, 1, false);
}

void VRRoadNetwork::setTerrainOffset(float o) { roadTerrainOffset = o; }
void VRRoadNetwork::setMarkingsWidth(float w) { markingsWidth = w; }
float VRRoadNetwork::getTerrainOffset() { return roadTerrainOffset; }
float VRRoadNetwork::getMarkingsWidth() { return markingsWidth; }

VREntityPtr VRRoadNetwork::addRoute(vector<int> nodeIDs) { // nodeIDs as computed from pathFinding
    vector<VREntityPtr> nodes;
    vector<Vec3d> norms;

    for (unsigned int i=0; i<nodeIDs.size()-1; i++) {
        int n1 = nodeIDs[i];
        int n2 = nodeIDs[i+1];
        int eID = graph->getEdgeID(n1, n2);

        auto eNorms = graphNormals[eID];
        auto lane = graphEdgeEntities[eID];
        auto nodes = lane->getEntity("path")->getAllEntities("nodes");

        VREntityPtr node1 = 0;
        VREntityPtr node2 = 0;

        for (auto nE : nodes) {
            auto n = nE->getEntity("node");
            if (n->getValue<int>("graphID", 0) == n1) node1 = n;
            if (n->getValue<int>("graphID", 0) == n2) node2 = n;
        }

        if (i == 0) {
            nodes.push_back(node1);
            norms.push_back(eNorms[0]);
        }

        nodes.push_back(node2);
        norms.push_back(eNorms[1]);
    }

    return addPath("Path", "route", nodes, norms);
}
template <class Key, class Value>
unsigned long mapSize(const map<Key,Value> &map){
    unsigned long size = 0;
    for(auto it = map.begin(); it != map.end(); ++it){
        size += sizeof(it->first);
        size += sizeof(it->second);
    }
    return size;
}

/*
vector<VRRoadPtr> roads;
vector<VRRoadPtr> ways;
vector<VRRoadIntersectionPtr> intersections;
vector<VRGeometryPtr> assets;
vector<VRTunnelPtr> tunnels;
vector<VRBridgePtr> bridges;

map<int, vector<Vec3d> > graphNormals;
map<int, VREntityPtr> graphEdgeEntities;
map<VREntityPtr, VRRoadPtr> roadsByEntity;
map<Vec4i, int> arrowTemplates;

GraphPtr graph;
VRPathtoolPtr tool;
VRAsphaltPtr asphalt;
VRAsphaltPtr asphaltArrow;
VRTexturePtr arrowTexture;
VRGeometryPtr arrows;
VRGeometryPtr collisionMesh;
*/

bool VRRoadNetwork::isIntersection(VREntityPtr isec){
    if (intersectionsByEntity.count(isec)) return true;
    return false;
}

void VRRoadNetwork::toggleGraph(){
    isShowingGraph = !isShowingGraph;
    map<int,int> idx;
	map<int,int> idx2;
	map<string, VRGeometryPtr> vizGeos;
	//auto graph = getGraph();
	auto scene = VRScene::getCurrent();
	for (string strInput : {"RoadNetworkPoints", "RoadNetworkLines", "RoadNetworkRelations", "RoadNetworkIntersec"}) {
		if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
		auto graphViz = VRGeometry::create(strInput);
        graphViz->setPersistency(0);
		addChild(graphViz);
		vizGeos[strInput] = graphViz;
	}
	string gAnn = "RoadNetworkAnnotation";
	if (scene->getRoot()->find(gAnn)) scene->getRoot()->find(gAnn)->destroy();
    if (!isShowingGraph) return;
	auto graphAnn = VRAnnotationEngine::create(gAnn);
	graphAnn->setPersistency(0);
	graphAnn->setBillboard(true);
	graphAnn->setBackground(Color4f(1,1,1,1));
	addChild(graphAnn);

	VRGeoData gg0;
	VRGeoData gg1;
	VRGeoData gg2;
	VRGeoData gg3;

	for (auto node : graph->getNodes()){
		auto nPose = graph->getNode(node.first).p;
		auto p = nPose.pos() + Vec3d(0,2,0);
		int vID = gg0.pushVert(p);
		gg1.pushVert(p);
		gg3.pushVert(p);
		gg0.pushPoint();
		graphAnn->set(vID, nPose.pos() + Vec3d(0,2.2,0), "Node "+toString(node.first));
		idx[node.first] = vID;
	}

	for (auto& connection : graph->getEdges()) {
		auto& edge = connection.second;
		int eID = edge.ID;
		if (eID < 0) continue;
		if (!getLaneSegment(eID)->getEntity("nextIntersection")) gg1.pushLine(idx[connection.second.from], idx[connection.second.to]);
		else gg3.pushLine(idx[connection.second.from], idx[connection.second.to]);
		auto pos1 = graph->getNode(connection.second.from).p.pos();
		auto pos2 = graph->getNode(connection.second.to).p.pos();
		graphAnn->set(eID+100, (pos1+pos2)*0.5 + Vec3d(0,2.6,0), "Edge "+toString(eID)+"("+toString(connection.second.from)+"-"+toString(connection.second.to)+")");
	}

    for (auto connection : graph->getEdges()){
		auto edge = connection.first;
		for (auto rel : graph->getRelations(edge)) {
            auto pos1 = (graph->getNode(connection.second.from).p.pos()+graph->getNode(connection.second.to).p.pos())/2 + Vec3d(0,2,0);
            auto pos2 = (graph->getNode(graph->getEdgeCopyByID(rel).from).p.pos()+graph->getNode(graph->getEdgeCopyByID(rel).to).p.pos())/2 + Vec3d(0,2,0);
            int pID1 = gg2.pushVert(pos1);
            int pID2 = gg2.pushVert(pos2);
            gg2.pushLine(pID1,pID2);
            //graphAnn->set(edge+100, (pos1+pos2)*0.5 + Vec3d(0,4,0), "Edge "+toString(edge)+"("+toString(connection.second.from)+"-"+toString(connection.second.to)+")");
		}
	}

	gg0.apply( vizGeos["RoadNetworkPoints"] );
	gg1.apply( vizGeos["RoadNetworkLines"] );
	gg2.apply( vizGeos["RoadNetworkRelations"] );
	gg3.apply( vizGeos["RoadNetworkIntersec"] );

	for (auto geo : vizGeos) {
		auto mat = VRMaterial::create(geo.first+"_mat");
		mat->setLit(0);
		int r = (geo.first ==  "RoadNetworkRelations" || geo.first == "RoadNetworkIntersec");
		int g = (geo.first == "RoadNetworkPoints" || geo.first ==  "RoadNetworkRelations"|| geo.first == "RoadNetworkIntersec");
		int b = (geo.first == "RoadNetworkLines"|| geo.first ==  "RoadNetworkRelations"|| geo.first == "RoadNetworkIntersec");
		mat->setDiffuse(Color3f(r,g,b));
		mat->setLineWidth(3);
		mat->setPointSize(5);
		geo.second->setMaterial(mat);
	}
}

double VRRoadNetwork::getMemoryConsumption() {
    double res = sizeof(*this);

    res += mapSize(graphNormals);
    res += mapSize(graphEdgeEntities);
    res += mapSize(roadsByEntity);
    res += mapSize(arrowTemplates);

    //for (auto& w : ways) if (w) res += w->getMemoryConsumption();

    res += asphalt->getMemoryConsumption();
    res += asphaltArrow->getMemoryConsumption();

    return res/1048576.0;
}

void VRRoadNetwork::update() {
    for (auto i : intersections) i->update();
}





