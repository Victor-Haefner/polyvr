#include "VRRoad.h"
#include "VRRoadNetwork.h"
#include "VRRoadIntersection.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "../assets/StreetLamp.h"
#include "core/utils/toString.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRStroke.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;


VRRoad::VRRoad() : VRRoadBase("Road") {}
VRRoad::~VRRoad() {}

VRRoadPtr VRRoad::create() {
    auto r = VRRoadPtr( new VRRoad() );
    r->hide("SHADOW");
    return r;
}

float VRRoad::getWidth() {
    float width = 0;
    for (auto lane : entity->getAllEntities("lanes")) width += toFloat( lane->get("width")->value );
    return width;
}

VREntityPtr VRRoad::getNodeEntry( VREntityPtr node ) {
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

bool VRRoad::hasMarkings() {
    string type = "road";
    if (auto t = getEntity()->get("type")) type = t->value;
    return (type != "unclassified" && type != "service" && type != "footway");
}

PosePtr VRRoad::getRightEdge(Vec3d pos) {
    auto path = toPath(getEntity()->getEntity("path"), 12);
    float t = path->getClosestPoint(pos); // get nearest road path position to pos
    auto pose = path->getPose(t);
    float offsetter = 0.0;
    if (t < 0.1) offsetter = offsetIn;
    if (t > 0.9) offsetter = offsetOut;
    Vec3d p = pose->pos() + pose->x()*(getWidth()*0.5 + offsetter);
    pose->setPos(p);
    return pose;
}

PosePtr VRRoad::getLeftEdge(Vec3d pos) {
    auto path = toPath(getEntity()->getEntity("path"), 12);
    float t = path->getClosestPoint(pos); // get nearest road path position to pos
    auto pose = path->getPose(t);
    float offsetter = 0.0;
    if (t < 0.1) offsetter = offsetIn;
    if (t > 0.9) offsetter = offsetOut;
    Vec3d p = pose->pos() - pose->x()*(getWidth()*0.5 + offsetter);
    pose->setPos(p);
    return pose;
}

PosePtr VRRoad::getSplit(Vec3d pos) {
    int n1 = 0;
    int n2 = 0;
    if (auto o = ontology.lock()) {
        auto roadEnt = getEntity();
        for (auto laneEnt : roadEnt->getAllEntities("lanes")) {
            auto laneDir = laneEnt->getValue("direction", 1);
            if ( laneDir > 0 ) n1++;
            if ( laneDir < 0 ) n2++;
        }
    }

    auto path = toPath(getEntity()->getEntity("path"), 12);
    float t = path->getClosestPoint(pos); // get nearest road path position to pos
    auto pose = path->getPose(t);
    float offsetter = 0.0;
    if (t < 0.1) offsetter = offsetIn;
    if (t > 0.9) offsetter = offsetOut;
    Vec3d p = pose->pos() + pose->x()*(getWidth()*0.5 + offsetter) - pose->x()*(getWidth()/(n1+n2)*n1 );
    pose->setPos(p);
    return pose;
}

PosePtr VRRoad::getPosition(float t) {
    vector<PathPtr> paths;
    for (auto p : entity->getAllEntities("path")) {
        paths.push_back( toPath(p,32) );
    }
    return paths[0]->getPose(t);
}

VRRoad::edgePoint& VRRoad::getEdgePoint( VREntityPtr node ) {
    if (edgePoints.count(node) == 0) {
        float width = getWidth();
        VREntityPtr rEntry = getNodeEntry( node );
        Vec3d norm = rEntry->getVec3("direction") * toInt(rEntry->get("sign")->value);
        Vec3d x = Vec3d(0,1,0).cross(norm);
        x.normalize();
        Vec3d pNode = node->getVec3("position");
        Vec3d p1 = pNode - x * 0.5 * width; // right
        Vec3d p2 = pNode + x * 0.5 * width; // left
        edgePoints[node] = edgePoint(p1,p2,norm,rEntry);
    }
    return edgePoints[node];
}

map<VREntityPtr, VRRoad::edgePoint>& VRRoad::getEdgePoints() {
    if (edgePoints.size() == 0) {
        auto e = getEntity();
        auto nodeEntries = e->getEntity("path")->getAllEntities("nodes");
        for (auto ne : nodeEntries) {
            auto n = ne->getEntity("node");
            getEdgePoint(n);
        }
    }
    return edgePoints;
}

vector<VRRoadPtr> VRRoad::splitAtIntersections(VRRoadNetworkPtr network) { // TODO: refactor the code a bit
    vector<VRRoadPtr> roads;
    auto e = getEntity();
    auto path = e->getEntity("path");
    auto nodes = path->getAllEntities("nodes");
    for (unsigned int i=1; i+1<nodes.size(); i++) {
        auto node = nodes[i]->getEntity("node");
        int N = node->getAllEntities("paths").size();
        if (N <= 1) continue;

        // shorten old road path and add nodes to new road path
        auto npath = path->copy();
        path->clear("nodes");
        npath->clear("nodes");
        for (unsigned int j=0; j<=i; j++) {
            path->add("nodes", nodes[j]->getName());
        }
        nodes[i]->set("sign", "1"); // last node of old path

        auto ne = nodes[i]->copy(); // new node entry
        ne->set("sign", "-1"); // first node of new path
        npath->add("nodes", ne->getName());
        ne->set("path", npath->getName());
        ne->getEntity("node")->add("paths", ne->getName());
        for (unsigned int j=i+1; j<nodes.size(); j++) {
            npath->add("nodes", nodes[j]->getName());
            nodes[j]->set("path", npath->getName());
        }

        // copy road
        int rID = network->getRoadID();
        auto r = create();
        r->setWorld(world.lock());
        auto re = e->copy();
        re->set("path", npath->getName());
        re->set("ID", toString(rID));
        r->setEntity(re);
        r->ontology = ontology;
        roads.push_back(r);

        re->clear("lanes");
        for (auto l : e->getAllEntities("lanes")) {
            auto lc = l->copy();
            lc->set("road", re->getName());
            re->add("lanes", lc->getName());
        }

        auto splits = r->splitAtIntersections(network);
        for (auto s : splits) roads.push_back(s);
        break;
    }
    return roads;
}

VRGeometryPtr VRRoad::createGeometry() {
    auto w = world.lock();
    if (!visible) return 0;
    if (!w) return 0;
    auto roads = w->getRoadNetwork();
    if (!roads) return 0;

    auto strokeGeometry = [&]() -> VRGeometryPtr {
	    float width = getWidth();
		float W = width*0.5;
		vector<Vec3d> profile;
		profile.push_back(Vec3d(-W,0,0));
		profile.push_back(Vec3d(W,0,0));

		auto geo = VRStroke::create("road");
		vector<PathPtr> paths;
		for (auto p : entity->getAllEntities("path")) {
            auto path = toPath(p,12);

            if(offsetIn!=0 || offsetOut!=0){
                for (int zz=0; zz<path->size(); zz++) {
                    Vec3d p = path->getPoint(zz).pos();
                    Vec3d n = path->getPoint(zz).dir();
                    Vec3d x = path->getPoint(zz).x();
                    auto po = path->getPoint(zz);
                    x.normalize();
                    float offsetter = offsetIn*(1.0-(float(zz)/(float(path->size())-1.0))) + offsetOut*(float(zz)/(float(path->size())-1.0));
                    if (zz>0 && zz+1<(int)path->getPoints().size()) offsetter = 0; //only first node has offsetter
                    po.setPos(x*offsetter  + p);
                    path->setPoint(zz,po);
                };
                path->compute(12);
            }
            paths.push_back( path );
		}
		geo->setPaths( paths );
		geo->strokeProfile(profile, 0, 0);
		if (!geo->getMesh()) return 0;
		if (auto t = terrain.lock()) t->elevateVertices(geo, roads->getTerrainOffset());
		return geo;
	};

	auto geo = strokeGeometry();
	if (!geo) return 0;
	setupTexCoords( geo, entity );
	//addChild(geo);
	selfPtr = geo;
    return geo;
}

VRGeometryPtr VRRoad::getGeometry() {
    if (selfPtr) return selfPtr;
    return 0;
}

void VRRoad::computeMarkings() {
    if (!hasMarkings()) return;
    auto w = world.lock();
    if (!w) return;
    auto roads = w->getRoadNetwork();
    if (!roads) return;
    float mw = roads->getMarkingsWidth();

    // road data
    vector<VREntityPtr> nodes;
    vector<Vec3d> normals;
    VREntityPtr pathEnt = entity->getEntity("path");
    if (!pathEnt) return;

    VREntityPtr nodeEntryIn = pathEnt->getEntity("nodes",0);
    VREntityPtr nodeEntryOut = pathEnt->getEntity("nodes",1);
    VREntityPtr nodeIn = nodeEntryIn->getEntity("node");
    VREntityPtr nodeOut = nodeEntryOut->getEntity("node");
    Vec3d normIn = nodeEntryIn->getVec3("direction");
    Vec3d normOut = nodeEntryOut->getVec3("direction");

    float roadWidth = getWidth();
    auto lanes = entity->getAllEntities("lanes");
    int Nlanes = lanes.size();

    auto add = [&](Vec3d pos, Vec3d n) {
        if (auto t = terrain.lock()) pos = t->elevatePoint(pos, 0.06);
        if (auto t = terrain.lock()) t->projectTangent(n, pos);
        nodes.push_back(addNode(0, pos));
        normals.push_back(n);
    };

    // compute markings nodes
    auto path = toPath(pathEnt, 12);
    int zz=0;
    for (auto point : path->getPoints()) {
        Vec3d p = point.pos();
        Vec3d n = point.dir();
        Vec3d x = point.x();
        x.normalize();
        float offsetter = offsetIn*(1.0-(float(zz)/(float(path->size())-1.0))) + offsetOut*(float(zz)/(float(path->size())-1.0));
        //float offsetterOld = offsetter;
        if (zz>0 && zz+1<(int)path->getPoints().size()) offsetter = 0; //only first node has offsetter
        float widthSum = -roadWidth*0.5 - offsetter;
        for (int li=0; li<Nlanes; li++) {
            auto lane = lanes[li];
            float width = toFloat( lane->get("width")->value );
            float k = widthSum;
            if (li == 0) k += mw*0.5;
            else if (lanes[li-1]->is_a("ParkingLane")) {
                k += mw*0.5;
            }
            add(-x*k + p, n);
            widthSum += width;
        }
        add(-x*(roadWidth*0.5 - mw*0.5 - offsetter ) + p, n);
        zz+=1;
    }

    // markings
    int pathN = path->size();
    int lastDir = 0;

    for (int li=0; li<Nlanes+1; li++) {
        vector<VREntityPtr> nodes2;
        vector<Vec3d> normals2;
        for (int pi=0; pi<pathN; pi++) {
            int i = pi*(Nlanes+1)+li;
            nodes2.push_back( nodes[i] );
            normals2.push_back( normals[i] );
        }
        auto mL = addPath("RoadMarking", name, nodes2, normals2);
        mL->set("width", toString(mw));
        entity->add("markings", mL->getName());

        if (li != Nlanes) {
            auto lane = lanes[li];
            if (!lane->is_a("Lane")) { lastDir = 0; continue; }

            // dotted lines between parallel lanes
            int direction = toInt( lane->get("direction")->value );
            if (li != 0 && lastDir*direction > 0) {
                mL->set("style", "dashed");
                mL->set("dashLength", "2");
            } else if (li != 0 && li != Nlanes-1) {
                mL->set("color", "yellow");
            }
            lastDir = direction;



            // parking lanes
            if (li > 0) if (lanes[li-1]->is_a("ParkingLane")) mL->set("dashLength", "1");

            if (lane->is_a("ParkingLane")) {
                auto linePath = toPath(mL, 12);
                if (!linePath) continue;
                int N = lane->getValue<int>("capacity", -1);
                float W = lane->getValue<float>("width", 0);
                for (int si = 0; si < N+1; si++) {
                    auto pose = linePath->getPose(si*1.0/N);
                    auto n = -pose->x();
                    n.normalize();
                    auto p1 = pose->pos();
                    if (si == 0) p1 += pose->dir()*mw*0.5;
                    if (si == N) p1 -= pose->dir()*mw*0.5;
                    auto p2 = p1 + n*W;

                    auto mL = addPath("RoadMarking", name, { addNode(0,p1), addNode(0,p2) }, {n,n} );
                    mL->set("width", toString(mw));
                    entity->add("markings", mL->getName());
                    mL->set("style", "solid");
                    mL->set("dashLength", "0");
                }
            }
        }
    }
}

void VRRoad::addParkingLane( int direction, float width, int capacity, string type ) {
    if (auto o = ontology.lock()) {
        auto l = o->addEntity( entity->getName()+"Lane", "ParkingLane");
        l->set("width", toString(width));
        l->set("direction", toString(direction));
        entity->add("lanes", l->getName());
        l->set("road", entity->getName());
        l->set("capacity", toString(capacity));
    }
}

void VRRoad::addTrafficLight( Vec3d pos ) {
    if (auto o = ontology.lock()) {
        auto roadEnt = getEntity();

        float dmin = 1e6;
        Vec3d dir; // direction from intersection to traffic light
        Vec3d as;
        Vec3d df;
        bool check = edgePoints.size() == 0;
        VREntityPtr nodeEnt;
        for (auto ep : getEdgePoints()) {
            Vec3d p = (ep.second.p1+ep.second.p2)*0.5;
            float d = (p-pos).length();
            if (dmin > d && d > 0.07) {
                dmin = d;
                dir = pos-p;
                dir.normalize();
                nodeEnt = ep.first;
                as = p;
                //df = pos;
            }
        }
        //cout << "  VRRoad::addTrafficLight " << toString(dmin) << "  " << toString(as) << "  " << toString(pos) << endl;
        for (auto laneEnt : roadEnt->getAllEntities("lanes")) {
            auto laneDir = laneEnt->getValue("direction", 1);
            Vec3d laneTangent = getRightEdge(pos)->dir() * laneDir;
            laneTangent.normalize();
            //cout << "   VRRoad::addTrafficLight " << toString(dir.dot(laneTangent)) << endl;
            if (dir.dot(laneTangent) < -0.5) {
                auto signalEnt = o->addEntity("trafficlight", "TrafficLight");
                laneEnt->add("signs",signalEnt->getName());
                signalEnt->add("lanes",laneEnt->getName());
                signalEnt->setVec3("position", pos, "Position");
                signalEnt->set("node", nodeEnt->getName());
                //cout << "   VRRoad::addTrafficLight- "<< "\033[32mset\033[0m" << " -" << signalEnt->getName() << endl;
            } //else cout << "   VRRoad::addTrafficLight-" << "\033[31mnot set\033[0m" << endl;

        }
        if ( check ) edgePoints.clear();
    }
}

void VRRoad::setOffsetIn(float o) { offsetIn = o; }
void VRRoad::setOffsetOut(float o) { offsetOut = o; }

void VRRoad::setVisible(bool in) { visible = in; }
bool VRRoad::isVisible() { return visible; }

void VRRoad::addIntersection(VRRoadIntersectionPtr isec) { intersections.push_back(isec); }
vector<VRRoadIntersectionPtr> VRRoad::getIntersections() { return intersections; }


