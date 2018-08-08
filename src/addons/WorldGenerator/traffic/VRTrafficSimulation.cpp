#include "VRTrafficSimulation.h"
#include "../roads/VRRoad.h"
#include "../roads/VRRoadNetwork.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/math/polygon.h"
#include "core/math/graph.h"
#include "core/math/triangulator.h"
#include "core/math/Octree.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRObjectManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/tools/VRAnnotationEngine.h"

#include <boost/bind.hpp>

#define CPRINT(x) \
VRGuiManager::get()->getConsole( "Console" )->write( string(x)+"\n" );

using namespace OSG;

template<class T>
void erase(vector<T>& v, const T& t) {
    v.erase(remove(v.begin(), v.end(), t), v.end());
}


VRTrafficSimulation::Vehicle::Vehicle(Graph::position p) : pos(p) {
    t = VRTransform::create("t");
    speed = speed*(1.0+0.2*0.01*(rand()%100));
    vehiclesight[INFRONT] = false;
    vehiclesight[FROMLEFT] = false;
    vehiclesight[FROMRIGHT] = false;
}

VRTrafficSimulation::Vehicle::Vehicle() {}
VRTrafficSimulation::Vehicle::~Vehicle() {}

void VRTrafficSimulation::Vehicle::hide() {
    t->hide();
}

void VRTrafficSimulation::Vehicle::show(Graph::position p) {
    pos = p;
    t->show();
}

int VRTrafficSimulation::Vehicle::getID() {
    return vID;
}

void VRTrafficSimulation::Vehicle::setID(int vID) {
    this->vID = vID;
}

void VRTrafficSimulation::Vehicle::destroy() {
    if (t) t->destroy();
    t = 0;
}

bool VRTrafficSimulation::Vehicle::operator==(const Vehicle& v) {
    return t == v.t;
}

VRTrafficSimulation::VRTrafficSimulation() : VRObject("TrafficSimulation") {
    updateCb = VRUpdateCb::create( "traffic", boost::bind(&VRTrafficSimulation::updateSimulation, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);

    auto box = VRGeometry::create("boxCar");
    box->setPrimitive("Box", "2 1.5 4 1 1 1");
    addVehicleModel(box);

    auto setupLightMaterial = [&](string name, Color3f c, bool lit) {
        auto l = VRMaterial::create(name);
        l->setLit(lit);
        l->setDiffuse(c);
        return l;
    };

    carLightWhiteOn   = setupLightMaterial("carLightWhiteOn"  , Color3f(1,1,1), false);
    carLightWhiteOff  = setupLightMaterial("carLightWhiteOff" , Color3f(0.5,0.5,0.5), true);
    carLightRedOn     = setupLightMaterial("carLightRedOn"    , Color3f(1,0.2,0.2), false);
    carLightRedOff    = setupLightMaterial("carLightRedOff"   , Color3f(0.5,0.1,0.1), true);
    carLightOrangeOn  = setupLightMaterial("carLightOrangeOn" , Color3f(1,0.7,0.1), false);
    carLightOrangeOff = setupLightMaterial("carLightOrangeOff", Color3f(0.5,0.35,0.05), true);
    carLightOrangeBlink = setupLightMaterial("carLightOrangeBlink", Color3f(1,0.7,0.1), false);

    turnSignalCb = VRUpdateCb::create( "turnSignal", boost::bind(&VRTrafficSimulation::updateTurnSignal, this) );
    VRScene::getCurrent()->addTimeoutFkt(turnSignalCb, 0, 500);
}

VRTrafficSimulation::~VRTrafficSimulation() {}

VRTrafficSimulationPtr VRTrafficSimulation::create() { return VRTrafficSimulationPtr( new VRTrafficSimulation() ); }

void VRTrafficSimulation::updateTurnSignal() {
    bool l = !carLightOrangeBlink->isLit();
    if (l) carLightOrangeBlink->setDiffuse(Color3f(0.5,0.35,0.05));
    else   carLightOrangeBlink->setDiffuse(Color3f(1,0.7,0.1));
    carLightOrangeBlink->setLit(l);
}

void VRTrafficSimulation::setRoadNetwork(VRRoadNetworkPtr rds) {
    roadNetwork = rds;
    roads.clear();
    auto graph = roadNetwork->getGraph();
    for (auto& e : graph->getEdgesCopy()) {
        int i = e.ID;
    //for (int i = 0; i < graph->getNEdges(); i++) {
        roads[i] = road();
        //auto& e = graph->getEdge(i);
        Vec3d p1 = graph->getNode(e.from).p.pos();
        Vec3d p2 = graph->getNode(e.to).p.pos();
        roads[i].length = (p2-p1).length();
        roads[i].rID = i;
    }

    //updateDensityVisual(true);
}

template<class T>
T randomChoice(vector<T> vec) {
    if (vec.size() == 0) return 0;
    auto res = vec[ round((float(random())/RAND_MAX) * (vec.size()-1)) ];
    return res;
}

void VRTrafficSimulation::updateSimulation() {
    if (!roadNetwork) return;
    auto g = roadNetwork->getGraph();
    auto space = Octree::create(2);
    map<int, vector<pair<int, int>>> toChangeRoad;
    map<int, int> toChangeLane;
    map<int, vector<vector<int>>> visionVec;
    float userRadius = 300; // x meter radius around users

    auto fillOctree = [&]() {
        for (auto& road : roads) { // fill octree
            for (auto& ID : road.second.vehicleIDs) {
                auto pos = vehicles[ID.first].t->getFrom();
                space->add(pos, &vehicles[ID.first]);
            }
        }
    };

    auto makeDiff = [&](vector<int>& v1, vector<int>& v2) {
        vector<int> res;
        for (auto oldID : v1) {
            bool old = true;
            for (auto newID : v2) if (newID == oldID) { old = false; break; }
            if (old) res.push_back(oldID);
        }
        return res;
    };

    auto updateSimulationArea = [&]() {
        // compare new and old list of roads in range -> remove vehicles on diff roads!

        auto graph = roadNetwork->getGraph();
        vector<int> newSeedRoads;
        vector<int> newNearRoads;

        for (auto user : users) {
            Vec3d p = getPoseTo(user.t)->pos();
            string debug = "";
            for (auto eV : graph->getEdges()) {
                auto& e = eV.second;
                if (debugOverRideSeedRoad<0 && graph->getPrevEdges(e).size() == 0) { // roads that start out of "nowhere"
                    newSeedRoads.push_back( e.ID );
                    continue;
                }

                Vec3d ep1 = graph->getNode(e.from).p.pos();
                Vec3d ep2 = graph->getNode(e.to  ).p.pos();
                float D1 = (ep1-p).length();
                float D2 = (ep2-p).length();

                if (D1 > userRadius && D2 > userRadius) continue; // outside
                ///TODO: look into radius
                if ( debugOverRideSeedRoad < 0 && (D1 > userRadius*0.5 || D2 > userRadius*0.5) ) newSeedRoads.push_back( e.ID ); // on edge
                newNearRoads.push_back( e.ID ); // inside or on edge
            }
            //cout << debug << endl;
        }

        if (debugOverRideSeedRoad!=-1) newSeedRoads.push_back( debugOverRideSeedRoad );

        for (auto roadID : makeDiff(nearRoads, newNearRoads)) {
            auto& road = roads[roadID];
            //for (auto v : road.vehicles) { v.destroy(); numUnits--; }
            for (auto ID : road.vehicleIDs) { vehicles[ID.first].hide(); vehiclePool.push_front(vehicles[ID.first]); numUnits--; }
            road.vehicleIDs.clear();
            road.macro = true;
        }
        if (forceSeedRoads.size() > 1) newSeedRoads = forceSeedRoads;
        seedRoads = newSeedRoads;
        nearRoads = newNearRoads;

        for (auto roadID : nearRoads) {
            auto& road = roads[roadID];
            road.macro = false;
        }

        string seedRoadsString = "seedRoads:";
        for (auto roadID : seedRoads) {
            auto& road = roads[roadID];
            //addVehicles(roadID, road.density, 1); // TODO: pass a vehicle type!!
            seedRoadsString += " " + toString(roadID);
        }

        //cout <<"random Seed " << toString(abs(float(rand())/float(RAND_MAX))) << endl;
        int rSeed  = int(abs(float(rand())/float(RAND_MAX) * seedRoads.size()));
        if (seedRoads.size()>0) {
            auto roadID = seedRoads[rSeed];
            auto& road = roads[roadID];
            addVehicle(roadID, road.density, 1);
        }

        if (seedRoadsString!=lastseedRoadsString) {
            //cout << seedRoadsString << endl;
            lastseedRoadsString = seedRoadsString;
        }
    };

    auto propagateVehicle = [&](Vehicle& vehicle, float d, int intention) {
        auto& gp = vehicle.pos;
        gp.pos += d;

        if (gp.pos > 1) {
            gp.pos -= 1;
            int road1ID = gp.edge;
            auto& edge = g->getEdge(gp.edge);
            auto nextEdges = g->getNextEdges(edge);
            if (nextEdges.size() > 1) {
                gp.edge = randomChoice(nextEdges).ID;
                auto& road = roads[gp.edge];
                if (road.macro) {
                    toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                }
                else {
                    toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                    if (vehicle.currentState != 0) {
                        vehicle.roadTo = g->getNextEdges(g->getEdge(vehicle.roadTo))[0].ID;
                        vehicle.roadFrom = gp.edge;
                    }
                }
                //cout << toString(gp.edge) << endl;
            }
            if (nextEdges.size() == 1) {
                gp.edge = nextEdges[0].ID;
                auto& road = roads[gp.edge];
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                if (vehicle.currentState != 0) {
                    vehicle.roadTo = g->getNextEdges(g->getEdge(vehicle.roadTo))[0].ID;
                    vehicle.roadFrom = gp.edge;
                }
                //cout << "  transit of vehicle: " << vehicle.getID() << " from: " << road1ID << " to: " << gp.edge << endl;
            }
            if (nextEdges.size() == 0) {
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                //cout << "   new spawn of vehicle: " << vehicle.getID() << endl; //" from: " << road1ID <<
            }
        }
        else {
            auto& edge = g->getEdge(gp.edge);
            auto nextEdges = g->getNextEdges(edge);
            //if (nextEdges.size() == 0) changeLane(vehicle.vID,1);
        }

        Vec3d offset = Vec3d(0,0,0);
        Vec3d doffset = Vec3d(0,0,0);
        Vec3d dir = vehicle.t->getPose()->dir();
        Vec3d up = vehicle.t->getPose()->up();
        Vec3d left = up.cross(dir);
        Vec3d right = -up.cross(dir);
        //cout << toString(left) << endl;
        float vS = float(vehicle.currentState);
        if (vS == 0) vehicle.currentOffset = Vec3d(0,0,0);
        if (vS == 0) vehicle.currentdOffset = Vec3d(0,0,0);
        if (intention==vehicle.STRAIGHT) { offset = vehicle.currentOffset + Vec3d(0,0,0); doffset = vehicle.currentdOffset + Vec3d(0,0,0); }
        if (intention==vehicle.SWITCHLEFT) { offset = vehicle.currentOffset + left*0.023; doffset = vehicle.currentdOffset + left*vS*0.0015; }
        if (intention==vehicle.SWITCHRIGHT) { offset = vehicle.currentOffset + right*0.023; doffset = vehicle.currentdOffset + -left*vS*0.0015; }
        //cout << toString(d*5*3) << " - " << toString(d*1.5) << endl;
        //cout << intention << " -- " << toString(vehicle.vID) << " -- " << toString(vehicle.behavior) <<  toString(vehicles[vehicle.vID].behavior) << " -- "<< toString(offset) << endl;

        auto road = roads[gp.edge];
        float lanewidth = 3.0; //TODO: should be called from real data
        float dirOffset;
        if (intention == 1 && vS > 0.5) dirOffset = left.dot(offset);
        if (intention == 2 && vS > 0.5) dirOffset = right.dot(offset);
        if (intention == 1 && vS < -0.5) dirOffset = right.dot(offset);
        if (intention == 2 && vS < -0.5) dirOffset = left.dot(offset);
        if (dirOffset > lanewidth/2 && vS>0.5) {
            toChangeRoad[vehicle.roadFrom].push_back( make_pair(vehicle.vID, vehicle.roadTo) );
            gp.edge = vehicle.roadTo;
            vehicle.currentState = -1;
            offset = -offset;
            //cout << "trafficsim changing state " << toString(vehicle.vID) << " " << toString(dirOffset) <<endl;
        }
        if (dirOffset < 0.1 && vS<-0.5) {
            for (auto l : vehicle.turnsignalsBL) l->setMaterial(carLightOrangeOff);
            for (auto l : vehicle.turnsignalsBR) l->setMaterial(carLightOrangeOff);
            for (auto l : vehicle.turnsignalsFL) l->setMaterial(carLightOrangeOff);
            for (auto l : vehicle.turnsignalsFR) l->setMaterial(carLightOrangeOff);
            vehicle.roadFrom = -1;
            vehicle.roadTo = -1;
            vehicle.currentState = 0;
            vehicle.behavior = 0;
            offset = Vec3d(0,0,0);
            doffset = Vec3d(0,0,0);
            //cout << "changed lane" << endl;
        }
        auto p = roadNetwork->getPosition(vehicle.pos);
        //cout << "propagated vehicle pos " <<toString(p) << endl;
        if (offset.length()>20) offset = Vec3d(0,0,0); //DEBUGGING FAIL SAFE
        vehicle.lastMove = p->pos() + offset - vehicle.t->getFrom();
        p->setPos(p->pos()+offset);
        p->setDir(p->dir()+doffset);
        vehicle.t->setPose(p);
        vehicle.lastMoveTS = VRGlobals::CURRENT_FRAME;
        vehicle.currentOffset = offset;
        vehicle.currentdOffset = doffset;

        int ran = rand() % 10000;
        if ( laneChange && ran >  0 && ran < 10 && !toChangeLane[vehicle.vID] ) { toChangeLane[vehicle.vID] = 1; }
        if ( laneChange && ran > 10 && ran < 20 && !toChangeLane[vehicle.vID] ) { toChangeLane[vehicle.vID] = 2; }
        //if (vehicle.vID == 0) cout << toString(offset) << endl;
        //cout << "Vehicle " << vehicle.vehicleID << " " << p->pos() << " " << vehicle.pos.edge << " " << vehicle.pos.pos << endl;
    };

    auto inFront = [&](PosePtr p1, PosePtr p2, Vec3d lastMove) -> bool { //vehicle1, vehicle2, vehicle1.lastmove
        lastMove.normalize();
        Vec3d D = p2->pos() - (p1->pos() + lastMove*3);
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d x = lastMove.cross(Vec3d(0,1,0));
        x.normalize();
        float rL = abs( D.dot(x) );

        return d > 0 && L < 5 && rL < 2;/*lastMove.normalize();
        Vec3d D = p2->pos() - (p1->pos() + lastMove*5); //vector between vehicles
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove); //check if vehicle2 behind vehicle1
        Vec3d x = lastMove.cross(Vec3d(0,1,0));
        x.normalize();
        float rL = abs( D.dot(x) ); //check if vehicle2 left or right of vehicle1

        return d > 0 && L < 5  && rL < 1.5; // in front, in range, in corridor*/
    };

    auto comingLeft = [&](PosePtr p1, PosePtr p2, Vec3d lastMove) -> bool { //vehicle1, vehicle2, vehicle1.lastmove
        lastMove.normalize();
        Vec3d D = p2->pos() - (p1->pos() + lastMove*3);
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d x = lastMove.cross(Vec3d(0,1,0));
        x.normalize();
        float r = Dn.dot(x);
        //float rL = abs( Dn.dot(x) );
        float a = -x.dot( p2->dir() );

        return d > 0 && L < 15 && r > 0 && a < -0.3/* && rL >= 2*/; // in front, right, crossing paths,

        /*lastMove.normalize();
        Vec3d D = p2->pos() - p1->pos();
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d left = p1->dir().cross(Vec3d(0,1,0)); //left vector
        left.normalize();
        float dis = abs( D.dot(p1->dir()) );
        float l = D.dot(left);
        //float rL = abs( Dn.dot(x) );
        float a = -left.dot( p2->dir() );
        bool leftBehind = d < 0 && dis < 5 && l > 0 && l < 4 ;
        bool leftFront = d > 0 && dis < 6 && l > 0 && l < 4;
        return leftBehind || leftFront;*/
    };

    auto comingRight = [&](PosePtr p1, PosePtr p2, Vec3d lastMove) -> bool {
        lastMove.normalize();
        Vec3d D = p2->pos() - (p1->pos() + lastMove*3);
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d x = lastMove.cross(Vec3d(0,1,0));
        x.normalize();
        float r = Dn.dot(x);
        //float rL = abs( Dn.dot(x) );
        float a = -x.dot( p2->dir() );

        return d > 0 && L < 15 && r > 0 && a > 0.3/* && rL >= 2*/; // in front, right, crossing paths,
        /*lastMove.normalize();
        Vec3d D = p2->pos() - p1->pos();
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d left = -p1->dir().cross(Vec3d(0,1,0)); //left vector
        left.normalize();
        float dis = abs( D.dot(p1->dir()) );
        float l = D.dot(left);
        //float rL = abs( Dn.dot(x) );
        float a = -left.dot( p2->dir() );
        bool rightBehind = d < 0 && dis < 5 && l > 0 && l < 4 ;
        bool rightFront = d > 0 && dis < 6 && l > 0 && l < 4 ;
        return rightBehind || rightFront;*/
    };

    auto propagateVehicles = [&]() {
        int N = 0;
        for (auto& road : roads) {
            for (auto& ID : road.second.vehicleIDs) {
                auto& vehicle = vehicles[ID.first];
                if (!vehicle.t) continue;
                vehicle.vehiclesight.clear();
                float d = speedMultiplier*vehicle.speed/road.second.length;
                if (!isSimRunning) d = 0;

                // check if road ahead is free
                auto pose = vehicle.t->getPose();
                auto res = space->radiusSearch(pose->pos(), 5);
                int state = 0;
                vector<int> visionIDsStraight;
                vector<int> visionIDsLeft;
                vector<int> visionIDsRight;
                for (auto vv : res) {
                    auto v = (Vehicle*)vv;
                    if (!v->t->isVisible()) continue;
                    if (!v) continue;
                    if (!v->t) continue;
                    auto p = v->t->getPose();

                    if (inFront(pose, p, vehicle.lastMove)) { state = 1; vehicle.vehiclesight[vehicle.INFRONT] = true; visionIDsStraight.push_back(v->vID); }
                    else if (comingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (comingLeft(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMLEFT] = true; visionIDsLeft.push_back(v->vID); }
                    if (comingRight(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMRIGHT] = true; visionIDsRight.push_back(v->vID); }
                    //if (state > 0) break;
                }
                visionVec[vehicle.vID].push_back(visionIDsStraight);
                visionVec[vehicle.vID].push_back(visionIDsLeft);
                visionVec[vehicle.vID].push_back(visionIDsRight);

                bool debugBool = false; ///Debugging
                if (debugBool) state = 0; //DANGER: debug mode, state = 0, discard collision check

                for (auto& v : users) {
                    auto p = v.t->getPose();
                    if (inFront(pose, p, vehicle.lastMove)) { state = 1; vehicle.vehiclesight[vehicle.INFRONT] = true; }
                    else if (comingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (comingLeft(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMLEFT] = true; }
                    if (comingRight(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMRIGHT] = true; }
                    //if (state > 0) break;
                }

                if (auto g = dynamic_pointer_cast<VRGeometry>(vehicle.mesh)) { // only for debugging!!
                    if (state == 0) g->setColor("white");
                    if (state == 1) g->setColor("blue");
                    if (state == 2) g->setColor("red");
                }

                int vbeh = vehicle.behavior;
                if ( vbeh == vehicle.STRAIGHT && !vehicle.vehiclesight[vehicle.INFRONT]) { propagateVehicle(vehicle, d, vbeh); }
                if ( vbeh == vehicle.STRAIGHT &&  vehicle.vehiclesight[vehicle.INFRONT]) { toChangeLane[vehicle.vID] = 1; }
                if ( vbeh == vehicle.SWITCHLEFT  && !vehicle.vehiclesight[vehicle.FROMLEFT] && !vehicle.vehiclesight[vehicle.INFRONT] /*&& VRGlobals::CURRENT_FRAME - vehicle.indicatorTS > 200*/ ) propagateVehicle(vehicle, d, vbeh);
                if ( vbeh == vehicle.SWITCHRIGHT && !vehicle.vehiclesight[vehicle.FROMRIGHT] && !vehicle.vehiclesight[vehicle.INFRONT] /*&& VRGlobals::CURRENT_FRAME - vehicle.indicatorTS > 200*/ ) propagateVehicle(vehicle, d, vbeh);

                if (VRGlobals::CURRENT_FRAME - vehicle.lastMoveTS > 200 ) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle.vID, -1) ); ///------killswitch if vehicle get's stuck
                }
                N++; // count vehicles!
            }
        }

        //cout << "propagateVehicles, updated " << N << " vehicles" << endl;
    };

    auto resolveRoadChanges = [&]() {
        for (auto r : toChangeRoad) {
            auto& road = roads[r.first];
            for (auto v : r.second) {
                road.vehicleIDs.erase(v.first);
                //if (v.second == -1) { v.first.destroy(); numUnits--; }
                if (v.second == -1) {
                    //vehicles[v.first].hide();
                    auto& vehicle = vehicles[v.first];
                    auto& gp = vehicle.pos;
                    auto p = roadNetwork->getPosition(vehicle.pos);
                    Vec3d offset = Vec3d(0,-30,0);
                    p->setPos(p->pos()+offset);
                    vehicle.t->setPose(p);
                    vehicle.hide();
                    gp.pos -= 1;
                    vehicle.currentState = 0;
                    vehicle.behavior = 0;
                    vehicle.roadFrom = -1;
                    vehicle.roadTo = -1;
                    vehicle.speed = 0.15;
                    vehiclePool.push_front(vehicles[v.first]);
                    for (auto l : vehicle.turnsignalsBL) l->setMaterial(carLightOrangeOff);
                    for (auto l : vehicle.turnsignalsBR) l->setMaterial(carLightOrangeOff);
                    for (auto l : vehicle.turnsignalsFL) l->setMaterial(carLightOrangeOff);
                    for (auto l : vehicle.turnsignalsFR) l->setMaterial(carLightOrangeOff);
                    numUnits--;
                }
                else {
                    auto& road2 = roads[v.second];
                    road2.vehicleIDs[v.first]=v.first;
                }
            }
        }
    };

    auto resolveLaneChanges = [&]() {
        for (auto l : toChangeLane) {
            changeLane(l.first,l.second);
        }
    };

    //map<int, map<int,vector<int>>> visionVec
    auto showVehicleVision = [&](){
        if (!isShowingVehicleVision) return;
        //VRGeometryPtr vizGeos;
        auto graph = roadNetwork->getGraph();
        auto scene = VRScene::getCurrent();

        string strInput = "graphVizVisionLines";
        if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
        auto graphViz = VRGeometry::create(strInput);
        graphViz->setPersistency(0);
        scene->getRoot()->addChild(graphViz);

        VRGeoData gg0;

        int n = 0;
        for (vv : visionVec){
            n = 0;
            for (vVis : vv.second){
                for (vvv : vVis){
                    if (vVis.size()==0) continue;
                    auto nPose1 = vehicles[vv.first].t->getPose();
                    auto nPose2 = vehicles[vvv].t->getPose();
                    auto p1 = nPose1->pos() + Vec3d(0,2+0.2*n,0.1*n);
                    auto p2 = nPose2->pos() + Vec3d(0,2+0.2*n,0.1*n);
                    int r = n==2 || n==0;
                    int g = n==0;
                    int b = n==1;
                    Color3f ff = Color3f(float(r),float(g),float(b));
                    int vID1 = gg0.pushVert(p1);
                    gg0.pushColor(Color3f(r,g,b));
                    int vID2 = gg0.pushVert(p2);
                    gg0.pushColor(Color3f(r,g,b));
                    gg0.pushLine(vID1,vID2);
                }
                n++;
            }
        }

        gg0.apply( graphViz );

        auto mat = VRMaterial::create(strInput+"_mat");
        mat->setLit(0);
        mat->setDiffuse(Color3f(1,1,0));
        mat->setLineWidth(3);
        graphViz->setMaterial(mat);
    };

    updateSimulationArea();
    fillOctree();
    propagateVehicles();
    //resolveCollisions();
    //updateDensityVisual();
    showVehicleVision();
    resolveRoadChanges();
    resolveLaneChanges();
}

void VRTrafficSimulation::addUser(VRTransformPtr t) {
    auto v = Vehicle( Graph::position(0, 0.0) );
    users.push_back(v);
    users[users.size()-1].t = t;
}

void VRTrafficSimulation::addVehicle(int roadID, float density, int type) {
    if (maxUnits > 0 && numUnits >= maxUnits) return;

    //roadcheck whether vehicle on seedroad in front of vehicle
    auto& road = roads[roadID];
    auto g = roadNetwork->getGraph();
    auto e = g->getEdge(roadID);
    int n1 = e.from;
    int n2 = e.to;
    float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
    float dis = 100000.0;
    if (road.vehicleIDs.size()>0) {
        auto pos = vehicles[road.lastVehicleID].t->getFrom();
        dis = (pos - g->getNode(n1).p.pos()).length();
        //cout << toString(pos)<< " --- "<< toString(g->getNode(n1).p.pos()) << " --- " << toString(dis)<< endl;
    } //TODO: change 5m below to vehicle length, maybe change 1m to safety distance between cars
    if (dis < (1+5.0/density)) return;  // density of 1 means one car per 6 meter!

    numUnits++;
    auto getVehicle = [&]() {
        if (vehiclePool.size() > 0) {
            auto v = vehiclePool.back();
            vehiclePool.pop_back();
            vehicles[v.vID].show( Graph::position(roadID, 0.0) );
            return v.vID;
        } else {
            auto v = Vehicle( Graph::position(roadID, 0.0) );
            v.mesh = models[type]->duplicate();
            for (auto obj : v.mesh->getChildren(true, "Geometry")) {
                VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
                string name = geo->getBaseName();
                if (name == "TurnSignalBL" || name == "turnsignalBL") v.turnsignalsBL.push_back(geo);
                if (name == "TurnSignalBR" || name == "turnsignalBR") v.turnsignalsBR.push_back(geo);
                if (name == "TurnSignalFL" || name == "turnsignalFL") v.turnsignalsFL.push_back(geo);
                if (name == "TurnSignalFR" || name == "turnsignalFR") v.turnsignalsFR.push_back(geo);
                if (name == "Headlight" || name == "headlight") v.headlights.push_back(geo);
                if (name == "Backlight" || name == "backlight") v.backlights.push_back(geo);
            }

            for (auto l : v.turnsignalsBL) l->setMaterial(carLightOrangeOff);
            for (auto l : v.turnsignalsBR) l->setMaterial(carLightOrangeOff);
            for (auto l : v.turnsignalsFL) l->setMaterial(carLightOrangeOff);
            for (auto l : v.turnsignalsFR) l->setMaterial(carLightOrangeOff);
            for (auto l : v.headlights) l->setMaterial(carLightWhiteOn);
            for (auto l : v.backlights) l->setMaterial(carLightRedOff);
            if (v.getID()==-1) {
                static size_t nID = -1; nID++;
                v.setID(nID);
                vehicles[nID] =v;
                //cout<<"VRTrafficSimulation::addVehicle: Added vehicle to map vID:" << v.getID()<<endl;
            }
            return v.vID;
        }
    };

    //if () cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    //auto& road = roads[roadID];
    Vehicle v = vehicles[getVehicle()];

    //if (VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(v.mesh) ) g->makeUnique(); // only for debugging!!
    //v.t->setPickable(true);

    v.t->addChild(v.mesh);
    addChild(v.t);
    road.vehicleIDs[v.getID()] = v.getID();
    road.lastVehicleID = v.getID();
}

void VRTrafficSimulation::addVehicles(int roadID, float density, int type) {
    auto road = roads[roadID];
    auto g = roadNetwork->getGraph();
    auto e = g->getEdge(roadID);
    int n1 = e.from;
    int n2 = e.to;
    float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
    int N0 = road.vehicleIDs.size();
    int N = L*density/5.0; // density of 1 means one car per 5 meter!
    //cout << "addVehicles N0 " << N0 << " L " << L << " d " << density << " N " << N << " to " << roadID << endl;
    for (int i=N0; i<N; i++) addVehicle(roadID, density, type);
}


/** CHANGE LANE **/
void VRTrafficSimulation::changeLane(int ID, int direction) {
    auto& v = vehicles[ID];
    if (v.behavior != 0) return;
    auto& gp = v.pos;
    auto& edge = roadNetwork->getGraph()->getEdge(gp.edge);
    auto posV = v.pos; //current vehicle position
    auto pos = v.pos; //position on alternative road
    auto poseV = roadNetwork->getPosition(gp);
    auto vDir = v.t->getPose()->dir();
    auto vUp = v.t->getPose()->up();
    bool checked = false;
    auto rSize = edge.relations.size();

    int edgeLeft;
    int edgeRight;
    auto check = [&](int input) {
        auto rSize = edge.relations.size();
        //CPRINT(toString(rSize));
        if (rSize > 0) {
            auto opt = edge.relations[0];
            if (!roadNetwork->getGraph()->hasEdge(opt)) return false;
            pos.edge = opt;
            auto pose = roadNetwork->getPosition(pos);
            float res = vUp.cross(vDir).dot(pose->pos() - poseV->pos());
            if (res > 0 && input==1) { edgeLeft = opt; return true; }
            if (res < 0 && input==2) { edgeRight = opt; return true; }

            if (rSize > 1) {
                opt = edge.relations[1];
                if (!roadNetwork->getGraph()->hasEdge(opt)) return false;
                pos.edge = opt;
                pose = roadNetwork->getPosition(pos);
                res = vUp.cross(vDir).dot(pose->pos() - poseV->pos());
                if (res > 0 && input==1) { edgeLeft = opt; return true; }
                if (res < 0 && input==2) { edgeRight = opt; return true; }
            }
        }
        return false;
    };

    auto signalLights = [&](int input) {
        if (input == 1) {
            for (auto l : v.turnsignalsBL) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.turnsignalsFL) l->setMaterial(carLightOrangeBlink);
        }
        if (input == 2) {
            for (auto l : v.turnsignalsBR) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.turnsignalsFR) l->setMaterial(carLightOrangeBlink);
        }
    };

    if ( direction == 1 && check(1) ) { checked = true; v.roadTo = edgeLeft; v.speed += 0.03; /* signalLights(1); */ }
    if ( direction == 2 && check(2)) { checked = true; v.roadTo = edgeRight; v.speed -= 0.03; /* signalLights(2); */ }
    if ( checked ){
        v.currentState = 1;
        v.behavior = direction;
        v.roadFrom = gp.edge;
        v.indicatorTS = VRGlobals::CURRENT_FRAME;
    }
    else {/*
        string s = toString(ID) + " " + toString(rSize) + " " + toString(poseV) + " rejected";
        CPRINT(s);
        cout << "VRTrafficSimulation::changeLane: trying to change to a lane, which doesn't exist" << endl;*/
    }
}

void VRTrafficSimulation::setTrafficDensity(float density, int type, int maxUnits) {
    //for (auto road : roads) addVehicles(road.first, density, type);
    this->maxUnits = maxUnits;
    for (auto& road : roads) road.second.density = density;
}

int VRTrafficSimulation::addVehicleModel(VRObjectPtr mesh) {
    models.push_back( mesh->duplicate() );
    return models.size()-1;
}

///DIAGNOSTICS
void VRTrafficSimulation::toggleSim() {
    isSimRunning = !isSimRunning;
}

void VRTrafficSimulation::setSpeedmultiplier(float speedMultiplier) {
    this->speedMultiplier = speedMultiplier;
}

/** SHOW GRAPH */
void VRTrafficSimulation::showGraph(){
	map<int,int> idx;
	map<int,int> idx2;
	map<string, VRGeometryPtr> vizGeos;
	auto graph = roadNetwork->getGraph();
	auto scene = VRScene::getCurrent();
	for (string strInput : {"graphVizPnts", "graphVizLines", "graphVizSeedLines", "graphVizRelations",}) {
		if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
		auto graphViz = VRGeometry::create(strInput);
        graphViz->setPersistency(0);
		addChild(graphViz);
		vizGeos[strInput] = graphViz;
	}

	string gAnn = "graphAnn";
	if (scene->getRoot()->find(gAnn)) scene->getRoot()->find(gAnn)->destroy();
	auto graphAnn = VRAnnotationEngine::create(gAnn);
	graphAnn->setPersistency(0);
	graphAnn->setBillboard(true);
	graphAnn->setBackground(Color4f(1,1,1,1));
	addChild(graphAnn);

	VRGeoData gg0;
	VRGeoData gg1;
	VRGeoData gg2;
	VRGeoData gg3;

	for (node : graph->getNodes()){
		auto nPose = graph->getNode(node.first).p;
		auto p = nPose.pos() + Vec3d(0,3,0);
		int vID = gg0.pushVert(p);
		gg1.pushVert(p);
		gg2.pushVert(p);
		gg0.pushPoint();
		graphAnn->set(vID, nPose.pos() + Vec3d(0,4,0), "Node "+toString(node.first));
		idx[node.first] = vID;
	}

	for (connection : graph->getEdges()){
		auto edge = connection.first;
		if (isSeedRoad(edge)) { gg2.pushLine(idx[connection.second.from], idx[connection.second.to]); }
		if (!isSeedRoad(edge)) { gg1.pushLine(idx[connection.second.from], idx[connection.second.to]); }
		auto pos1 = graph->getNode(connection.second.from).p.pos();
		auto pos2 = graph->getNode(connection.second.to).p.pos();
		graphAnn->set(edge+100, (pos1+pos2)*0.5 + Vec3d(0,4,0), "Edge "+toString(edge)+"("+toString(connection.second.from)+"-"+toString(connection.second.to)+")");
	}

    for (connection : graph->getEdges()){
		auto edge = connection.first;
		for (rel : graph->getRelations(edge)) {
            //if (isSeedRoad(edge)) { gg2.pushLine(idx[connection.second.from], idx[connection.second.to]); }
            auto pos1 = (graph->getNode(connection.second.from).p.pos()+graph->getNode(connection.second.to).p.pos())/2 + Vec3d(0,3,0);
            auto pos2 = (graph->getNode(graph->getEdgeCopyByID(rel).from).p.pos()+graph->getNode(graph->getEdgeCopyByID(rel).to).p.pos())/2 + Vec3d(0,3,0);
            int pID1 = gg3.pushVert(pos1);
            int pID2 = gg3.pushVert(pos2);
            gg3.pushLine(pID1,pID2);
            //graphAnn->set(edge+100, (pos1+pos2)*0.5 + Vec3d(0,4,0), "Edge "+toString(edge)+"("+toString(connection.second.from)+"-"+toString(connection.second.to)+")");
		}
	}

	gg0.apply( vizGeos["graphVizPnts"] );
	gg1.apply( vizGeos["graphVizLines"] );
	gg2.apply( vizGeos["graphVizSeedLines"] );
	gg3.apply( vizGeos["graphVizRelations"] );

	for (auto geo : vizGeos) {
		auto mat = VRMaterial::create(geo.first+"_mat");
		mat->setLit(0);
		int r = (geo.first == "graphVizSeedLines" || geo.first ==  "graphVizRelations");
		int g = (geo.first == "graphVizPnts" || geo.first ==  "graphVizRelations");
		int b = (geo.first == "graphVizLines"|| geo.first ==  "graphVizRelations");
		mat->setDiffuse(Color3f(r,g,b));
		mat->setLineWidth(3);
		mat->setPointSize(5);
		geo.second->setMaterial(mat);
	}
}

void VRTrafficSimulation::hideGraph(){
    vector<string> gg;
	gg.push_back("graphVizPnts");
	gg.push_back("graphVizLines");
	gg.push_back("graphVizSeedLines");
	gg.push_back("graphVizRelations");
	gg.push_back("graphAnn");
	auto scene = VRScene::getCurrent();
    for ( a : gg ){
        if ( scene->getRoot()->find(a) ) scene->getRoot()->find(a)->destroy();
    }
}

/** VEHICLE VISION*/
void VRTrafficSimulation::showVehicVision(){
    isShowingVehicleVision = true;
}

void VRTrafficSimulation::hideVehicVision(){
    isShowingVehicleVision = false;
}

string VRTrafficSimulation::getVehicleData(int ID){
    string res = "";
    string nl = "\n ";
    //auto car = allVehicles[ID];
    //auto car =
    //res+= car.vID;
    //res+= "- " + toString(car.pos);
    /*int counter=0;
    for (auto car : vecVehicles) {
        counter++;
    }
    res+="Number of Vehicles: " + toString(counter) + nl;*/
    auto v = vehicles[ID];
    res+= "VehicleID: " + toString(v.getID());
    res+= nl + " position: " + toString(v.t->getFrom());
    res+= nl + " vehiclesight: " + nl +  " INFRONT:" + toString(v.vehiclesight[v.INFRONT]) + " FROMLEFT: " + toString(v.vehiclesight[v.FROMLEFT]) + " FROMRIGHT:" + toString(v.vehiclesight[v.FROMRIGHT]);

    return res;
}

void VRTrafficSimulation::forceIntention(int vID,int behavior){
    //vehicles[vID].behavior = behavior;
    changeLane(vID,behavior);
    //cout << "Vehicle " << toString(vID) << " set to" << vehicles[vID].behavior << endl;
}

void VRTrafficSimulation::runDiagnostics(){
    string returnInfo = "";
    string nl = "\n ";
    string roadInfo = "ALL ROADS: ";
    string edgeInfo = "ALL EDGES: ";
    string nodeInfo = "ALL NODES: ";
    string vehiInfo = "ALL VEHICLES: ";
    string edgeNeighbors = "Relations: ";

    auto fit = [&](int input) {
        string res = "";
        int l1 = toString(input).length();
        for (int i=l1 ; i<4 ; i++) res+=" ";
        return res+toString(input);
    };

    ///get all roads, edges
    int n1 = 0;
    int n2 = 0;
    auto graph = roadNetwork->getGraph();
    for (auto eV : graph->getEdges()) {
        auto e = eV.second;
        roadInfo += toString(e.ID) + " ";
        edgeInfo += toString(e.from) +"-"+ toString(e.to) + " ";
        if (graph->getRelations(e.ID).size() > 0) {
            edgeNeighbors += toString(e.ID) + ":";
            for (nn : graph->getRelations(e.ID)) { edgeNeighbors +=" " + toString(nn); }
            edgeNeighbors += "; ";
            n2++;
        }
        n1++;
    }

    ///get all nodes
    int n3 = 0;
    for (auto n : graph->getNodes()) {
        nodeInfo += toString(n.second.ID) + " ";
        n3++;
    }

    ///get all vehicles
    int n4=0;
    for (auto v : vehicles) {
        vehiInfo += toString(v.second.vID) + " ";
        n4++;
    }

    returnInfo += lastseedRoadsString;
    returnInfo += nl + fit(n1) + "--" + roadInfo;
    returnInfo += nl + fit(n1) + "--" + edgeInfo;
    returnInfo += nl + fit(n2) + "--" + edgeNeighbors;
    returnInfo += nl + fit(n3) + "--" + nodeInfo;
    returnInfo += nl + fit(n4) + "--" + vehiInfo;


    CPRINT(returnInfo);
}

bool VRTrafficSimulation::isSeedRoad(int roadID){
    for (auto e : seedRoads){if (roadID==e) return true;}
    return false;
}

void VRTrafficSimulation::setSeedRoad(int debugOverRideSeedRoad){
    this->debugOverRideSeedRoad = debugOverRideSeedRoad;
}

void VRTrafficSimulation::setSeedRoadVec(vector<int> forceSeedRoads){
    this->forceSeedRoads = forceSeedRoads;
}

void VRTrafficSimulation::toggleLangeChanges(){
    laneChange = !laneChange;
    CPRINT("lanechanging: "+toString(laneChange));
}

void VRTrafficSimulation::updateDensityVisual(bool remesh) {
    if (!flowGeo) {
        flowGeo = VRGeometry::create("trafficFlow");
        addChild(flowGeo);

        auto mat = VRMaterial::create("trafficFlow");
        mat->setLit(0);
        mat->setLineWidth(5);
        flowGeo->setMaterial(mat);
    }

    if (roadNetwork && remesh) {
        VRGeoData geo;
        float h = 2;
        auto g = roadNetwork->getGraph();
        for (auto n : g->getNodes()) geo.pushVert( n.second.p.pos() + Vec3d(0,h,0) );
        for (auto e : g->getEdges()) geo.pushLine(e.second.from, e.second.to);
        geo.apply(flowGeo);
    }

    if (flowGeo->size() > 0) { // TODO
        //for (auto road)
    }
}



