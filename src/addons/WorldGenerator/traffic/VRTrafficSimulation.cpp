#include "VRTrafficSimulation.h"
#include "VRTrafficLights.h"
#include "../roads/VRRoad.h"
#include "../roads/VRRoadNetwork.h"
#include "../roads/VRRoadIntersection.h"
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
#include <GL/glut.h>

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
    //speed = speed*(1.0+0.2*0.01*(rand()%100));
    setDefaults();

    //speed = speed*(1.0+0.2*0.01*(rand()%100));
    vehiclesight[INFRONT] = -1.0;
    vehiclesight[FRONTLEFT] = -1.0;
    vehiclesight[FRONTRIGHT] = -1.0;
    vehiclesight[BEHINDLEFT] = -1.0;
    vehiclesight[BEHINDRIGHT] = -1.0;
    vehiclesight[BEHIND] = -1.0;
}

VRTrafficSimulation::Vehicle::Vehicle() {}
VRTrafficSimulation::Vehicle::~Vehicle() {}

void VRTrafficSimulation::Vehicle::hide() {
    t->hide();
}

void VRTrafficSimulation::Vehicle::setDefaults() {
    currentVelocity = 0.0;
    targetVelocity = 50.0/3.6; //try m/s  - km/h
    float tmp = targetVelocity;
    targetVelocity = targetVelocity*(1.0+0.2*0.01*(rand()%100));
    roadVelocity = targetVelocity;
    distanceToNextIntersec = 10000;

    maxAcceleration = 5;
    maxAcceleration += (targetVelocity - tmp)/5;
    maxDecceleration = 10; //8 dry, 4-5 sand, 1-4 snow
    acceleration = 0.0;
    decceleration = 0.0;

    pos.pos = 0;
    behavior = 0; //0 = straight, 1 = left, 2 = right
    currentState = 0; //0 = on lane, 1 = leaving lane, -1 = coming onto lane
    roadFrom = -1;
    roadTo = -1;
    nextEdge = -1;

    lastMove = Vec3d(0,0,0);
    currentOffset = Vec3d(0,0,0);
    currentdOffset = Vec3d(0,0,0);

    speed = targetVelocity;
    currentVelocity = 0.0;
    collisionDetected = false;

    signalAhead = false;
    nextSignalState = "000"; //red|organge|green
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
    box->setPrimitive("Box 2 1.5 4 1 1 1");
    addVehicleModel(box);

    auto setupLightMaterial = [&](string name, Color3f c, bool lit) {
        auto l = VRMaterial::create(name);
        l->setLit(lit);
        l->setDiffuse(c);
        return l;
    };

    roadVelocity = 50.0/3.6;

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
        roads[i] = laneSegment();
        //auto& e = graph->getEdge(i);
        Vec3d p1 = graph->getNode(e.from).p.pos();
        Vec3d p2 = graph->getNode(e.to).p.pos();
        roads[i].length = (p2-p1).length();
        roads[i].rID = i;

        /*auto laneEnt = roadNetwork->getLane(i);
        for (auto sign : laneEnt->getAllEntities("signs")) { // TODO
            VREntityPtr node = sign->getEntity("node");
            Vec3d pos = sign->getVec3("position");
            Graph::position gp = graph->getGraphPosition(pos);

            signal sig;
            sig.position = gp.t;
            sig.type = "blaa";
            roads[i].signals.push_back(sig);
        }*/
    }
    /** PSEUDO
    for (auto road : roadNetwork->getRoads()) {
        for (auto lane : road->getLaneEntites()) {
            for (auto eID : lane->getAllValues<int>("graphIDS")) {
                roads[eID].lane = lane;
                roads[eID].road = road;
            }
        }
    }
    */
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
    map<int, map<int, int>> visionVec;
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

        auto isPedestrian = [&](int ID) {
            auto lane = roadNetwork->getLane(ID);
            if (lane->getValue<bool>("pedestrian", false)) return true;
            return false;
        };

        auto isParkingLane = [&](int ID) {
            auto lane = roadNetwork->getLane(ID);
            if (lane->is_a("ParkingLane")) return true;
            return false;
        };

        for (auto user : users) {
            Vec3d p = getPoseTo(user.t)->pos();
            string debug = "";
            for (auto eV : graph->getEdges()) {
                auto& e = eV.second;

                Vec3d ep1 = graph->getNode(e.from).p.pos();
                Vec3d ep2 = graph->getNode(e.to  ).p.pos();
                float D1 = (ep1-p).length();
                float D2 = (ep2-p).length();

                if (D1 > userRadius && D2 > userRadius) continue; // outside
                if (debugOverRideSeedRoad<0 && graph->getPrevEdges(e).size() == 0  && !isPedestrian(e.ID) && !isParkingLane(e.ID)) { // roads that start out of "nowhere"
                    newSeedRoads.push_back( e.ID );
                    continue;
                }
                ///TODO: look into radius
                if ( debugOverRideSeedRoad > -2 && debugOverRideSeedRoad < 0 && (D1 > userRadius*0.5 || D2 > userRadius*0.5) && !isPedestrian(e.ID) ) newSeedRoads.push_back( e.ID ); // on edge
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
            if (isShowingGraph) updateGraph();
        }
    };

    auto propagateVehicle = [&](Vehicle& vehicle, float d, int intention) {
        auto& gp = vehicle.pos;
        gp.pos += d;

        if (gp.pos > 0.3 && vehicle.nextEdge == -1) {
            auto& edge = g->getEdge(gp.edge);
            auto nextEdges = g->getNextEdges(edge);

            if (nextEdges.size() > 1)  { vehicle.nextEdge = randomChoice(nextEdges).ID; }
            if (nextEdges.size() == 1) { vehicle.nextEdge = nextEdges[0].ID; }
            if (nextEdges.size() == 0) { vehicle.nextEdge = -1; }
        }

        if (gp.pos > 1) {
            gp.pos -= 1;
            int road1ID = gp.edge;
            auto& edge = g->getEdge(gp.edge);
            auto nextEdges = g->getNextEdges(edge);

            if (nextEdges.size() > 1) {
                gp.edge = vehicle.nextEdge;
                //gp.edge = randomChoice(nextEdges).ID;
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
                    gp.pos = gp.pos * roads[road1ID].length/roads[gp.edge].length;
                }
                //cout << toString(gp.edge) << endl;
            }
            if (nextEdges.size() == 1) {
                gp.edge = vehicle.nextEdge;
                //gp.edge = nextEdges[0].ID;
                auto& road = roads[gp.edge];
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                gp.pos = gp.pos * roads[road1ID].length/roads[gp.edge].length;
                if (vehicle.currentState != 0) {
                    if (g->getNextEdges(g->getEdge(vehicle.roadTo)).size() < 1) toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                    else {
                        vehicle.roadTo = g->getNextEdges(g->getEdge(vehicle.roadTo))[0].ID;
                        vehicle.roadFrom = gp.edge;
                    }
                }
                //cout << "  transit of vehicle: " << vehicle.getID() << " from: " << road1ID << " to: " << gp.edge << endl;
            }
            if (nextEdges.size() == 0) {
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                //cout << "   new spawn of vehicle: " << vehicle.getID() << endl; //" from: " << road1ID <<
            }
            vehicle.nextEdge = -1;
        }
        else {
            //auto& edge = g->getEdge(gp.edge);
            //auto nextEdges = g->getNextEdges(edge);
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
        //float offsetVel = 0.023;
        //float doffsetVel = 0.0015;
        float offsetVel = 0.023;
        float doffsetVel = 0.0015;
        if ( vehicle.currentVelocity < 25 ) { offsetVel = 0.023*2.0; doffsetVel = 0.0015*2.0; }
        if (intention==vehicle.STRAIGHT) { offset = vehicle.currentOffset + Vec3d(0,0,0); doffset = vehicle.currentdOffset + Vec3d(0,0,0); }
        if (intention==vehicle.SWITCHLEFT) { offset = vehicle.currentOffset + left*offsetVel; doffset = vehicle.currentdOffset + left*vS*doffsetVel; }
        if (intention==vehicle.SWITCHRIGHT) { offset = vehicle.currentOffset + right*offsetVel; doffset = vehicle.currentdOffset + -left*vS*doffsetVel; }
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
            vehicle.nextEdge = -1;
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
        //if ( laneChange && ran >  0 && ran < 1000 && !toChangeLane[vehicle.vID] ) { toChangeLane[vehicle.vID] = 1; }
        //if ( laneChange && ran > 1000 && ran < 2000 && !toChangeLane[vehicle.vID] ) { toChangeLane[vehicle.vID] = 2; }
        //if (vehicle.vID == 0) cout << toString(offset) << endl;
        //cout << "Vehicle " << vehicle.vehicleID << " " << p->pos() << " " << vehicle.pos.edge << " " << vehicle.pos.pos << endl;
    };
    /*
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

        return d > 0 && L < 5  && rL < 1.5; // in front, in range, in corridor*
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

        return d > 0 && L < 15 && r > 0 && a < -0.3/* && rL >= 2*; // in front, right, crossing paths,
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

        return d > 0 && L < 15 && r > 0 && a > 0.3/* && rL >= 2*; // in front, right, crossing paths,
    };*/

    auto farPosition  = [&](int ID, PosePtr p1, PosePtr p2, float disToM, Vec3d lastMove) -> int {
        auto& vehicle = vehicles[ID];
        //Vec3d D = p2->pos() - (p1->pos() + lastMove*5); //vector between vehicles
        Vec3d D = p2->pos() - p1->pos();
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove); //check if vehicle2 behind vehicle1
        //Vec3d x = lastMove.cross(Vec3d(0,1,0));
        Vec3d x = p1->dir().cross(Vec3d(0,1,0));
        x.normalize();
        //float rL = abs( D.dot(x) ); //check if vehicle2 left or right of vehicle1   rL < 1
        float left = - D.dot(x);

        if ( d > 0 && disToM < vehicle.width/2 ) return INFRONT; // in front, in range, in corridor
        if ( d < 0 && disToM < vehicle.width/2 ) return BEHIND; // in behind, in range, in corridor
        if ( d > 0 && left > 1 && left < 5) return FRONTLEFT; // in front, in range, left of corridor
        if ( d > 0 && left <-1 && left >-5) return FRONTRIGHT; // in front, in range, right of corridor
        if ( d < 0 && left > 1 && left < 5) return BEHINDLEFT; // in behind, in range, left of corridor
        if ( d < 0 && left <-1 && left >-5) return BEHINDRIGHT; // in behind, in range, right of corridor
        return -1;
    };

    auto calcFramePoints = [&](Vehicle& vehicle) {
    //calculation of 4 points at the edges of vehicle
        if (vehicle.lastFPTS == VRGlobals::CURRENT_FRAME) return;
        auto p = vehicle.t->getPose();
        if (vehicle.isUser) {
            Vec3d offset = Vec3d(-4817.3,0,-5559.75);
            p->setPos(p->pos() - offset);
        }
        auto dir = p->dir();
        dir.normalize();
        auto left = p->up().cross(dir);
        auto LH = dir*0.5*vehicle.length; //half of vehicle length
        auto WH = left*0.5*vehicle.width;   //half of vehicle width
        vehicle.vehicleFPs[0] = p->pos() + LH + WH;
        vehicle.vehicleFPs[1] = p->pos() + LH - WH;
        vehicle.vehicleFPs[2] = p->pos() - LH + WH;
        vehicle.vehicleFPs[3] = p->pos() - LH - WH;
        vehicle.vehicleFPs[4] = p->pos();
        vehicle.lastFPTS = VRGlobals::CURRENT_FRAME;
    };

    auto calcDisToFP = [&](Vehicle& v1, Vehicle& v2) {
    //simple way to calculate the distance between vehicle1-FPs and vehicle2-middle track
    ///TODO: make B-curve later AGRAJAG
        float res = 5000.0;
        auto dir = v1.t->getPose()->dir();
        for (auto p : v2.vehicleFPs) {
            float cc = abs((p.second - v1.t->getPose()->pos()).dot(dir.cross(v1.t->getPose()->up())));
            if (res > cc) res = cc;
        }
        return res;
    };

    auto computePerception = [&](Vehicle& vehicle) {
        auto setSight = [&](int dir, float D, int ID) {
        //set nearest vehicleID as neighbor, also set Distance
            if (dir==-1) return;
            if (!vehicle.vehiclesightFar.count(dir)) {
                vehicle.vehiclesightFar[dir] = D;
                vehicle.vehiclesightFarID[dir] = ID;
                return;
            }
            if ( vehicle.vehiclesightFar[dir] > 0 && D < vehicle.vehiclesightFar[dir] ) {
                vehicle.vehiclesightFar[dir] = D;
                vehicle.vehiclesightFarID[dir] = ID;
            }
        };

        vehicle.vehiclesight.clear();
        if (isSimRunning){
            vehicle.vehiclesightFar.clear();
            vehicle.vehiclesightFarID.clear();
        }

        float safetyDis = vehicle.currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + 6;
        float sightRadius = safetyDis + 8;

        auto pose = vehicle.t->getPose();
        auto resFar = space->radiusSearch(pose->pos(), sightRadius);

        for (auto vv : resFar) {
        //check vehicles in radiusSearch
            auto v = (Vehicle*)vv;
            if (!v->t->isVisible()) continue;
            if (!v) continue;
            if (!v->t) continue;
            auto p = v->t->getPose();
            auto D = (pose->pos() - p->pos()).length();

            calcFramePoints(vehicles[v->vID]);
            float diss = calcDisToFP(vehicle,vehicles[v->vID]); //distance to middle line of vehicle
            int farP = farPosition(vehicle.vID, pose, p, diss, vehicle.lastMove);
            setSight(farP,D,v->vID);
            //if (vehicle.currentVelocity > 6 && D <  0.1) vehicle.collisionDetected = true;
        }

        for (auto& v : users) {
            if (!v.t) continue;
            //auto vP = v.t->getPoseTo( this->getWorldPose() );
            auto p = v.t->getWorldPose();
            Vec3d offset = Vec3d(-4817.3,0,-5559.75); ///AGRAJAG - should be globalOffset
            p->setPos(p->pos() - offset);
            auto simpleDis = (pose->pos() - p->pos()).length();
            if (simpleDis > safetyDis + 10) continue;
            calcFramePoints(v);
            float diss = calcDisToFP(vehicle,v);
            int farP = farPosition(vehicle.vID, pose, p, diss, vehicle.lastMove);
            setSight(farP,simpleDis,v.vID);
        }

        visionVec[vehicle.vID] = vehicle.vehiclesightFarID;

        //========================

        VRTrafficLightPtr nextSignalE;
        Vec3d nextSignalP;

        function<bool (VREntityPtr, int, Vec3d)> recSearch =[&](VREntityPtr newLane, int eID, Vec3d posV) {
            auto interE = roadNetwork->getIntersection(newLane->getEntity("nextIntersection"));
            if (interE) {
                auto node = newLane->getEntity("nextIntersection")->getEntity("node");
                Vec3d pNode = node->getVec3("position");

                auto type = newLane->getEntity("nextIntersection")->get("type")->value;
                auto g = roadNetwork->getGraph();
                if (type != "intersection" && g->getNextEdges(g->getEdge(eID)).size()==1) {
                    if ( (pNode - posV).length()<safetyDis + 50 ) {
                        auto nextID = g->getNextEdges(g->getEdge(eID))[0].ID;
                        auto nextLane = roadNetwork->getLane(nextID);
                        recSearch (nextLane, nextID, posV);
                    }
                    else { vehicle.signalAhead = false; return false; }
                }
                if (type == "intersection") {
                    nextSignalE = interE->getTrafficLight(newLane);
                    vehicle.distanceToNextIntersec = (pNode - posV).dot(vehicle.t->getPose()->dir());

                    ///-----------------------------------------------------------------------------------------
                    auto g = roadNetwork->getGraph();
                    auto& edge = g->getEdge(eID);
                    auto nextEdges = g->getNextEdges(edge);

                    if (nextEdges.size() > 1) {
                        for (auto a : nextEdges) {
                            if (a.ID == vehicle.nextEdge) {
                                auto road1 = roadNetwork->getRoad(roadNetwork->getLane(eID));
                                auto road2 = roadNetwork->getRoad(roadNetwork->getLane(a.ID));
                                //auto& data1 = road1->getEdgePoint( node );
                                //auto& data2 = road2->getEdgePoint( node );
                                /*float connectingAngle = data1.n.dot(data2.n);
                                Vec3d w = data1.n.cross(data2.n);
                                float a = asin( w.length() );
                                if (w[1] < 0) a = -a;
                                float turnLeft = a;

                                bool parallel  = bool( connectingAngle < -0.8 );
                                bool left  = bool( turnLeft < 0);
                                if (  parallel ) { vehicle.turnAhead = 0; }
                                if ( !parallel &&  left ) { vehicle.turnAhead = 1; }
                                if ( !parallel && !left ) { vehicle.turnAhead = 2; }*/
                            }
                        }
                    }
                    ///-----------------------------------------------------------------------------------------

                    if (nextSignalE) {
                        nextSignalP = nextSignalE->getFrom();
                        vehicle.distanceToNextSignal  = (nextSignalP - posV).dot(vehicle.t->getPose()->dir());
                        vehicle.nextSignalState = nextSignalE->getState();
                        vehicle.signalAhead = true;
                    }
                    return true;
                }
            }
            vehicle.signalAhead = false;
            return false;
        };

        auto laneE = roadNetwork->getLane(vehicle.pos.edge);
        auto nextIntersectionE = roadNetwork->getIntersection(laneE->getEntity("nextIntersection"));

        vehicle.distanceToNextIntersec = 10000;
        recSearch(laneE,vehicle.pos.edge,vehicle.t->getPose()->pos());
    };

    auto propagateVehicles = [&]() {
        int N = 0;
        float current = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
        deltaT = current - lastT;
        lastT = current;
        for (auto& road : roads) {
            for (auto& ID : road.second.vehicleIDs) {
                auto& vehicle = vehicles[ID.first];
                if (!vehicle.t) continue;

                computePerception(vehicle);
                //computeDecision(vehicle);
                //computeAction(vehicle);

                float d = vehicle.currentVelocity;
                float safetyDis = vehicle.currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + 6;
                int vbeh = vehicle.behavior;
                float accFactor = vehicle.maxAcceleration;
                float decFactor = - vehicle.maxDecceleration;

                auto checkL = [&](int ID) -> bool { //
                    float disFL = 1000;
                    float disFR = 1000;
                    float disF  = 1000;
                    float disBL = 1000;
                    float safetyDis2 = safetyDis;

                    if ( vehicles[ID].vehiclesightFar[FRONTLEFT]>0 )    disFL = vehicles[ID].vehiclesightFar[FRONTLEFT];
                    if ( vehicles[ID].vehiclesightFar[FRONTRIGHT]>0 )   disFR = vehicles[ID].vehiclesightFar[FRONTRIGHT];
                    if ( vehicles[ID].vehiclesightFar[INFRONT]>0 )      disF  = vehicles[ID].vehiclesightFar[INFRONT];
                    if ( vehicles[ID].vehiclesightFar[BEHINDLEFT]>0 )  {
                        disBL = vehicles[ID].vehiclesightFar[BEHINDLEFT];
                        safetyDis2 = vehicles[vehicles[ID].vehiclesightFarID[BEHINDLEFT]].currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + 6;
                    }
                    return disFL > safetyDis && disBL > safetyDis2 && disF > safetyDis;
                    return false;
                };

                auto checkR = [&](int ID) -> bool { //
                    float disFR = 1000;
                    float disBR = 1000;
                    float disF = 1000;
                    float safetyDis2 = safetyDis;

                    if ( vehicles[ID].vehiclesightFar[FRONTRIGHT]>0 )     disFR = vehicles[ID].vehiclesightFar[FRONTRIGHT];
                    //if ( vehicles[ID].vehiclesightFar[BEHINDRIGHT]>0 )    disBR = vehicles[ID].vehiclesightFar[BEHINDRIGHT];
                    if ( vehicles[ID].vehiclesightFar[INFRONT]>0 )        disF  = vehicles[ID].vehiclesightFar[INFRONT];
                    if ( vehicles[ID].vehiclesightFar[BEHINDRIGHT]>0 )  {
                        disBR = vehicles[ID].vehiclesightFar[BEHINDRIGHT];
                        safetyDis2 = vehicles[vehicles[ID].vehiclesightFarID[BEHINDRIGHT]].currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + 6;
                    }
                    //float disB = vehicles[ID].vehiclesightFar[BEHIND];
                    return disFR > safetyDis && disBR > safetyDis2 && disF > safetyDis;
                    return false;
                };

                float nextMove = (vehicle.currentVelocity) * deltaT;
                float nextMoveAcc = (vehicle.currentVelocity + accFactor*deltaT) * deltaT;
                float nextMoveDec = (vehicle.currentVelocity + decFactor*deltaT) * deltaT;
                float sinceLastLS = VRGlobals::CURRENT_FRAME - vehicle.lastLaneSwitchTS;

                bool signalBlock = (vehicle.nextSignalState=="100" || vehicle.nextSignalState=="010");
                bool interBlock = (vehicle.distanceToNextIntersec < 60 && vehicle.distanceToNextIntersec > 10);
                //if (nextSignal != "000") cout << toString(nextSignal) << endl;

                /*
                Vec3d left = vehicle.t->getPose()->up().cross(vehicle.lastMove);
                left.normalize();
                Vec3d dir = vehicle.t->getWorldPose()->dir();
                dir.normalize();
                Vec3d tmp = vehicle.lastMove;
                tmp.normalize();
                bool checkLeft = left.dot(dir) > 0;
                cout << toString(vehicle.t->getPose()->up()) << endl;
                if ( (checkLeft && vehicle.distanceToNextIntersec < 10) || (vehicle.distanceToNextIntersec < 10 && vehicle.turnAtIntersec) ) vehicle.turnAtIntersec = true;
                else vehicle.turnAtIntersec = false;*/

                vehicle.targetVelocity = roadVelocity;
                if ( vehicle.distanceToNextIntersec < 20 ) vehicle.targetVelocity = 20/3.6;
                if ( vehicle.distanceToNextIntersec < 10 ) vehicle.targetVelocity = 15/3.6;
                if ( vehicle.distanceToNextIntersec > 20 ) vehicle.targetVelocity = 50/3.6;

                auto inFront = [&]() { return vehicle.vehiclesightFarID.count(INFRONT); };
                auto comingLeft = [&]() { return vehicle.vehiclesightFarID.count(FRONTLEFT); };
                auto comingRight = [&]() { return vehicle.vehiclesightFarID.count(FRONTRIGHT); };
                auto accelerate = [&]() { d = vehicle.currentVelocity + accFactor*deltaT; };
                auto decelerate = [&]() { d = vehicle.currentVelocity + decFactor*deltaT; };
                auto holdVelocity = [&]() { d = vehicle.currentVelocity; };
                //auto nextVehicleTurner = [&]() { return (vehicles[vehicle.vehiclesightFarID[INFRONT]].turnAtIntersec || vehicles[vehicle.vehiclesightFarID[FRONTRIGHT]].turnAtIntersec); };

                //if ( nextEdges.size() > 1 ) { nextIntersection = (1 - vehicle.pos.pos) * roads[vehicle.pos.edge].length; cout << toString(nextIntersection) << endl; }

                auto behave = [&]() {
                ///LOGIC
                    float nextSignalDistance = vehicle.distanceToNextSignal;
                    float nextIntersection = vehicle.distanceToNextIntersec;
                    bool signalAhead = vehicle.signalAhead;
                    bool turnAhead = vehicle.turnAhead>0;
                    turnAhead = true;

                    //if ( nextIntersection < 20 && turnAhead ) { vehicle.targetVelocity = 20/3.6; }
                    //if ( nextIntersection < 15 && turnAhead ) { vehicle.targetVelocity = 10/3.6; }
                    //if ( vehicle.turnAtIntersec ) { d = 0; return; vehicle.targetVelocity = 0; }
                    if (roadNetwork->getLane(vehicle.pos.edge)->get("turnDirection")) {
                        //cout << roadNetwork->getLane(vehicle.pos.edge)->get("turnDirection")->value << endl;
                        //if (toString(roadNetwork->getLane(vehicle.pos.edge)->get("turnDirection")->value) == "left") { d = 0; return; }
                    }

                    if ( vbeh == vehicle.STRAIGHT ) {
                        if ( ( signalAhead && nextSignalDistance < safetyDis -4 ) && signalBlock ) { decelerate(); return; }
                        if ( !signalAhead && nextSignalDistance < safetyDis -4 && signalBlock ) { decelerate(); return; }

                        if ( !inFront() ) {
                        //no vehicle ahead
                            //if ( vehicle.turnAtIntersec && ( comingRight() || comingLeft() ) && !nextVehicleTurner() ) { decelerate(); return; }
                            if ( sinceLastLS > 200 && nextIntersection > 20 && checkR(vehicle.vID) && vehicle.currentVelocity > 20 ) { toChangeLane[vehicle.vID] = 2; }
                            //check if road is ending/intersections etc - possible breaking
                            if ( vehicle.currentVelocity < vehicle.targetVelocity ) { accelerate(); return; }
                            //targetVelocity not reached //accelerate
                            if ( vehicle.currentVelocity > vehicle.targetVelocity ) { decelerate(); return; }
                            //targetVelocity overreached //decelerate
                            if ( vehicle.currentVelocity == vehicle.targetVelocity ) { holdVelocity(); return; }
                            //targetVelocity reached //proceed
                        }
                        if ( inFront() ) {
                        //vehicle ahead?
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            //if ( vehicle.turnAtIntersec && !nextVehicleTurner() ) { decelerate(); return; }
                            if ( disToFrontV - nextMove < safetyDis ) { decelerate(); return; }
                            //if ( nextIntersection < safetyDis + 0.2 ) { decelerate(); return; }
                            if ( vehicles[frontID].currentVelocity > vehicle.currentVelocity && vehicle.currentVelocity < vehicle.targetVelocity ) {
                            //vehicle ahead faster, and targetVelocity not reached
                                if ( disToFrontV - nextMoveAcc > safetyDis ) { accelerate(); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(); return; }
                                //break/decelerate
                            }
                            if ( vehicles[frontID].currentVelocity <= vehicle.currentVelocity && vehicle.currentVelocity < vehicle.targetVelocity ) {
                            //vehicle ahead slower
                                if ( sinceLastLS > 200 && nextIntersection > 20 && checkL(vehicle.vID) && vehicle.currentVelocity > 20/3.6 ) { toChangeLane[vehicle.vID] = 1; }
                                //check if lane switch possible
                                if ( disToFrontV - nextMoveAcc > safetyDis ) { accelerate(); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(); return; }
                                //break/decelerate
                            }
                            if ( vehicle.currentVelocity >= vehicle.targetVelocity ) {
                            //vehicle ahead slower
                                if ( sinceLastLS > 200 && nextIntersection > 20 && checkR(vehicle.vID) && vehicle.currentVelocity > 20/3.6 ) { toChangeLane[vehicle.vID] = 2; }
                                //check if lane switch possible
                                if ( disToFrontV - nextMove > safetyDis && vehicle.currentVelocity == vehicle.targetVelocity ) { holdVelocity(); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(); return; }
                                //break/decelerate
                            }
                        }
                    }
                    if ( vbeh == vehicle.SWITCHLEFT ) {
                        if ( !inFront() ) { holdVelocity(); return; }
                        //no vehicle ahead
                        if ( inFront() ) {
                        //vehicle ahead?
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            if ( disToFrontV - nextMove > safetyDis ) { accelerate(); return; }
                            //accelerate
                            if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(); return; }
                            //break/decelerate
                        }
                    }
                    if ( vbeh == vehicle.SWITCHRIGHT ) {
                        if ( !inFront() ) { holdVelocity(); return; }
                        //no vehicle ahead
                        if ( inFront() ) {
                        //vehicle ahead?
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            if ( disToFrontV - nextMove > safetyDis ) { accelerate(); return; }
                            //accelerate
                            if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(); return; }
                            //break/decelerate
                        }
                    }
                    if ( vbeh == vehicle.REVERSE ) {
                        d = vehicle.currentVelocity;
                    }
                };
                behave();
                vehicle.currentVelocity = d;
                d *= deltaT/road.second.length;
                if (d<0.0003 && vehicle.pos.pos > 0.1 && signalBlock) d = 0;
                //if (nextSignalDistance > 0.5 && nextSignalDistance < 5 && nextSignalState=="100") d = 0; //hack
                if (!isSimRunning) d = 0;
                if (stopVehicleID == ID.first) d = 0;
                if (isSimRunning && d<=0 && vbeh != vehicle.REVERSE) { d = 0; vehicle.currentVelocity = d; }
                if (vehicle.collisionDetected) d = 0;
                if (d!=0 && speedMultiplier!=1.0) d*=speedMultiplier;
                if (d!=0) propagateVehicle(vehicle, d, vbeh);



                /*
                if (auto g = dynamic_pointer_cast<VRGeometry>(vehicle.mesh)) { // only for debugging!!
                    if (state == 0) g->setColor("white");
                    if (state == 1) g->setColor("blue");
                    if (state == 2) g->setColor("red");
                }

                auto seesVehicle = [&](int dir) {
                    if (vehicle.vehiclesight[dir] == -1) return false;
                    return vehicle.vehiclesight[dir]>0;
                };
                //cout << toString(vehicle.vehiclesight[0]) << toString(vehicle.vehiclesight[1]) << toString(vehicle.vehiclesight[2]) << endl ;
                int vbeh = vehicle.behavior;
                bool inFront = seesVehicle(vehicle.INFRONT);
                if ( vbeh == vehicle.STRAIGHT && !inFront) { propagateVehicle(vehicle, d, vbeh); }
                if ( vbeh == vehicle.STRAIGHT &&  inFront) { toChangeLane[vehicle.vID] = 1; }*/
                //if ( vbeh == vehicle.SWITCHLEFT  && !seesVehicle(vehicle.FRONTLEFT) && !inFront /*&& VRGlobals::CURRENT_FRAME - vehicle.indicatorTS > 200*/ ) propagateVehicle(vehicle, d, vbeh);
                //if ( vbeh == vehicle.SWITCHRIGHT && !seesVehicle(vehicle.BEHINDRIGHT) && !inFront /*&& VRGlobals::CURRENT_FRAME - vehicle.indicatorTS > 200*/ ) propagateVehicle(vehicle, d, vbeh);
                if (isSimRunning && VRGlobals::CURRENT_FRAME - vehicle.lastMoveTS > 200 && !interBlock) { // && !interBlock && stopVehicleID != ID.first) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle.vID, -1) ); ///------killswitch if vehicle get's stuck
                }
                if (!isSimRunning) vehicle.lastMoveTS = VRGlobals::CURRENT_FRAME;
                N++; // count vehicles!
            }
        }
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
                    vehicle.hide();
                    auto& gp = vehicle.pos;
                    auto p = roadNetwork->getPosition(vehicle.pos);
                    Vec3d offset = Vec3d(0,-30,0);
                    p->setPos(p->pos()+offset);
                    vehicle.t->setPose(p);
                    //gp.pos -= 1;
                    gp.pos = 0;
                    vehicle.setDefaults();
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
        if (deleteVehicleID > -1) deleteVehicleID = -1;
    };

    auto resolveLaneChanges = [&]() {
        for (auto l : toChangeLane) {
            changeLane(l.first,l.second);
        }
    };

    //map<int, map<int, int>> visionVec
    //vehicID, visions, IDs
    auto showVehicleVision = [&](){
        if (!isShowingVehicleVision) return;
        auto graph = roadNetwork->getGraph();
        auto scene = VRScene::getCurrent();
        string strInput = "graphVizVisionLines";
        if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();

        VRGeoData gg0;
        if (visionVec.size()==0) return;
        for (auto vv : visionVec){
            for (auto vVis : vv.second){
                if (!vVis.second) continue;
                int n = vVis.first;
                //if (n == INFRONT) continue;
                //if (n == FRONTLEFT) continue;
                //if (n == FRONTRIGHT) continue;
                if (n == BEHINDLEFT) continue;
                if (n == BEHINDRIGHT) continue;
                if (n == BEHIND) continue;
                int ID1 = vv.first;
                int ID2 = vVis.second;

                if (!vehicles[ID1].t->isVisible() || !vehicles[ID2].t->isVisible()) continue;

                auto nPose1 = vehicles[ID1].t->getWorldPose();
                auto nPose2 = vehicles[ID2].t->getWorldPose();

                auto p1 = nPose1->pos() + Vec3d(0,2.2,0);
                auto p2 = nPose2->pos() + Vec3d(0,1.6,0);
                int vID1 = gg0.pushVert(p1);
                int r = n==4 || n==5;
                int g = n==2 || n==3;
                int b = n==1 || n==3 || n==5;
                gg0.pushColor(Color3f(r,g,b));

                int vID2 = gg0.pushVert(p2);
                gg0.pushColor(Color3f(r,g,b));
                gg0.pushLine(vID1,vID2);
            }
        }
        if (gg0.size() > 0) {
            auto graphViz = VRGeometry::create(strInput);
            graphViz->setPersistency(0);
            scene->getRoot()->addChild(graphViz);

            gg0.apply( graphViz );

            auto mat = VRMaterial::create(strInput+"_mat");
            mat->setLit(0);
            mat->setDiffuse(Color3f(1,1,0));
            mat->setLineWidth(3);
            graphViz->setMaterial(mat);
        }
    };

    auto showVehicleMarkers = [&](){
        if (isShowingGeometries) return;

        auto scene = VRScene::getCurrent();
        string strInput = "graphVizVehicMarkers";
        if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();

        string vAnn = "vehicAnn";
        if (scene->getRoot()->find(vAnn)) scene->getRoot()->find(vAnn)->destroy();
        auto vehicAnn = VRAnnotationEngine::create(vAnn);
        vehicAnn->setPersistency(0);
        vehicAnn->setBillboard(true);
        vehicAnn->setBackground(Color4f(1,1,1,1));
        addChild(vehicAnn);

        VRGeoData gg0;
        if (vehicles.size()==0) return;
        for (auto vv : vehicles){
            auto v = vv.second;
            //if (!v) continue;
            if (!v.t) continue;
            if (v.isUser) continue;
            auto vPose = v.t->getWorldPose();
            auto p = vPose->pos() + Vec3d(0,1.7,0);
            auto p2 = v.t->getPose()->pos() + Vec3d(0,1.7,0);
            int vID1 = gg0.pushVert(p);
            gg0.pushColor(Color3f(1,1,0));
            gg0.pushPoint();
            vehicAnn->set(vID1, p2 + Vec3d(0,0.2,0), "V: "+toString(v.getID()));
        }
        if (gg0.size() > 0) {
            auto graphViz = VRGeometry::create(strInput);
            graphViz->setPersistency(0);
            scene->getRoot()->addChild(graphViz);

            gg0.apply( graphViz );

            auto mat = VRMaterial::create(strInput+"_mat");
            mat->setLit(0);
            mat->setDiffuse(Color3f(1,1,0));
            mat->setLineWidth(3);
            mat->setPointSize(4);
            graphViz->setMaterial(mat);
        }
    };

    updateSimulationArea();
    fillOctree();
    propagateVehicles();
    //resolveCollisions();
    //updateDensityVisual();
    resolveRoadChanges();
    resolveLaneChanges();
    showVehicleVision();
    showVehicleMarkers();
}

void VRTrafficSimulation::addUser(VRTransformPtr t) {
    auto v = Vehicle( Graph::position(0, 0.0) );
    nID++;
    v.setID(nID);
    vehicles[nID] =v;
    v.isUser = true;
    users.push_back(v);
    users[users.size()-1].t = t;
    cout << "VRTrafficSimulation::addUser " << nID << endl;
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
                nID++;
                v.setID(nID);
                vehicles[nID] =v;
                //cout << nID << endl ;
                //cout<<"VRTrafficSimulation::addVehicle: Added vehicle to map vID:" << v.getID()<<endl;
            }
            return v.vID;
        }
    };

    //if () cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    //auto& road = roads[roadID];
    Vehicle v = vehicles[getVehicle()];
    vehicles[v.getID()].setDefaults();

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
            if (roadNetwork->getGraph()->getEdge(opt).ID == -1) return false;
            pos.edge = opt;
            auto pose = roadNetwork->getPosition(pos);
            if ((pose->pos() - poseV->pos()).length() > 3.5) return false;
            float res = vUp.cross(vDir).dot(pose->pos() - poseV->pos());
            if (res > 0 && input==1) { edgeLeft = opt; return true; }
            if (res < 0 && input==2) { edgeRight = opt; return true; }

            if (rSize > 1) {
                opt = edge.relations[1];
                if (roadNetwork->getGraph()->getEdge(opt).ID == -1) return false;
                pos.edge = opt;
                pose = roadNetwork->getPosition(pos);
                if ((pose->pos() - poseV->pos()).length() > 3.5) return false;
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
        v.lastLaneSwitchTS = VRGlobals::CURRENT_FRAME;
        //cout << "VRTrafficSimulation::changeLane" << toString(v.behavior) << " - " << toString(v.roadFrom) << " - " << toString(v.roadTo) << endl;
    }
    else {/*
        string s = toString(ID) + " " + toString(rSize) + " " + toString(poseV) + " rejected";
        CPRINT(s);*/
        //cout << "VRTrafficSimulation::changeLane: trying to change to a lane, which doesn't exist "<< toString(v.roadFrom) << " - " << toString(v.roadTo) << endl;
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
    isShowingGraph = true;
    updateGraph();
}

void VRTrafficSimulation::updateGraph(){
    map<int,int> idx;
	map<int,int> idx2;
	map<string, VRGeometryPtr> vizGeos;
	auto graph = roadNetwork->getGraph();
	auto scene = VRScene::getCurrent();
	for (string strInput : {"graphVizPnts", "graphVizLines", "graphVizMacroLines", "graphVizSeedLines", "graphVizRelations",}) {
		if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
		auto graphViz = VRGeometry::create(strInput);
        graphViz->setPersistency(0);
		addChild(graphViz);
		vizGeos[strInput] = graphViz;
	}
	string gAnn = "graphAnn";
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
	VRGeoData gg4;

	for (auto node : graph->getNodes()){
		auto nPose = graph->getNode(node.first).p;
		auto p = nPose.pos() + Vec3d(0,2,0);
		int vID = gg0.pushVert(p);
		gg1.pushVert(p);
		gg2.pushVert(p);
		gg4.pushVert(p);
		gg0.pushPoint();
		graphAnn->set(vID, nPose.pos() + Vec3d(0,2.2,0), "Node "+toString(node.first));
		idx[node.first] = vID;
	}

	for (auto connection : graph->getEdges()){
		auto edge = connection.first;
		auto& road = roads[edge];
		if (isSeedRoad(edge)) { gg2.pushLine(idx[connection.second.from], idx[connection.second.to]); }
		if (!isSeedRoad(edge) && !road.macro) { gg1.pushLine(idx[connection.second.from], idx[connection.second.to]); }
		if (road.macro) { gg4.pushLine(idx[connection.second.from], idx[connection.second.to]); }
		auto pos1 = graph->getNode(connection.second.from).p.pos();
		auto pos2 = graph->getNode(connection.second.to).p.pos();
		graphAnn->set(edge+100, (pos1+pos2)*0.5 + Vec3d(0,2.6,0), "Edge "+toString(edge)+"("+toString(connection.second.from)+"-"+toString(connection.second.to)+")");
	}

    for (auto connection : graph->getEdges()){
		auto edge = connection.first;
		for (auto rel : graph->getRelations(edge)) {
            //if (isSeedRoad(edge)) { gg2.pushLine(idx[connection.second.from], idx[connection.second.to]); }
            auto pos1 = (graph->getNode(connection.second.from).p.pos()+graph->getNode(connection.second.to).p.pos())/2 + Vec3d(0,2,0);
            auto pos2 = (graph->getNode(graph->getEdgeCopyByID(rel).from).p.pos()+graph->getNode(graph->getEdgeCopyByID(rel).to).p.pos())/2 + Vec3d(0,2,0);
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
	gg4.apply( vizGeos["graphVizMacroLines"] );

	for (auto geo : vizGeos) {
		auto mat = VRMaterial::create(geo.first+"_mat");
		mat->setLit(0);
		int r = (geo.first == "graphVizSeedLines" || geo.first ==  "graphVizRelations" || geo.first ==  "graphVizMacroLines");
		int g = (geo.first == "graphVizPnts" || geo.first ==  "graphVizRelations" || geo.first ==  "graphVizMacroLines");
		int b = (geo.first == "graphVizLines"|| geo.first ==  "graphVizRelations");
		mat->setDiffuse(Color3f(r,g,b));
		mat->setLineWidth(3);
		mat->setPointSize(5);
		geo.second->setMaterial(mat);
	}
}

void VRTrafficSimulation::hideGraph(){
    isShowingGraph = false;
    updateGraph();
}

/** SHOW INTERSECTION */
void VRTrafficSimulation::showIntersections(){ updateIntersectionVis(true); }
void VRTrafficSimulation::hideIntersections(){ updateIntersectionVis(false); }

void VRTrafficSimulation::updateIntersectionVis(bool in){
    map<int,int> idx;
	map<int,int> idx2;
	map<string, VRGeometryPtr> vizGeos;
	auto graph = roadNetwork->getGraph();
	auto scene = VRScene::getCurrent();
	for (string strInput : {"graphVizIntersections", "graphVizLightOrigin", "graphVizLightDir"}) {
		if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
		auto graphViz = VRGeometry::create(strInput);
        graphViz->setPersistency(0);
		addChild(graphViz);
		vizGeos[strInput] = graphViz;
	}
	string insecA = "insecAnn";
	if (scene->getRoot()->find(insecA)) scene->getRoot()->find(insecA)->destroy();
	string insecTA = "insecTypeAnn";
	if (scene->getRoot()->find(insecTA)) scene->getRoot()->find(insecTA)->destroy();

	if (!in) return;
	auto insecAnn = VRAnnotationEngine::create(insecA);
	auto insecTAnn = VRAnnotationEngine::create(insecTA);
	insecAnn->setPersistency(0);
	insecAnn->setBillboard(true);
	insecAnn->setBackground(Color4f(1,1,1,1));
	addChild(insecAnn);
	insecTAnn->setPersistency(0);
	insecTAnn->setBillboard(true);
	insecTAnn->setBackground(Color4f(1,1,1,1));
	addChild(insecTAnn);

	VRGeoData gg0; //intersection position
	VRGeoData gg1; //traffic signal origin points
	VRGeoData gg2; //traffic signal dirs
	//VRGeoData gg3; //traffic signal links

    auto intersections = roadNetwork->getIntersections();
	for (auto intersection : intersections){
        int pID0 = gg0.pushVert(intersection->getEntity()->getEntity("node")->getVec3("position")+Vec3d(0,5,0));
        gg0.pushPoint();
        string anno = toString(toString(intersection->getEntity()->get("type")->value));
        insecTAnn->set(pID0, intersection->getEntity()->getEntity("node")->getVec3("position")+Vec3d(0,5.5,0), anno);

        auto lights = intersection->getTrafficLights();
        for (auto light : lights){
            gg1.pushVert(light->getEntity()->getVec3("position")+Vec3d(0,3,0));
            gg1.pushPoint();
            Vec3d p1 = light->getPose()->pos();
            Vec3d p2 = p1 + light->getPose()->up()*0.5;
            int pID1 = gg2.pushVert(p1);
            int pID2 = gg2.pushVert(p2);
            gg2.pushLine(pID1,pID2);
            string anno = toString(light->getEntity()->getName()) + " " + toString(intersection->getEntity()->get("type")->value);
            insecAnn->set(pID1, light->getPose()->pos()+Vec3d(0,0.5,0), anno);
        }
        /*
        for (auto group : getTrafficLightMap) {
            for (auto light : group.second){
                Vec3d p = light1->getPose()->pos();
                int pID = gg3.pushVert(p1);
            }
            Vec3d p1 = light1->getPose()->pos();
            Vec3d p2 = light1->getPose()->pos();
            int pID1 = gg3.pushVert(p1);
            int pID2 = gg3.pushVert(p2);
            gg3.pushLine(pID1,pID2);
        }*/
	}

	gg0.apply( vizGeos["graphVizIntersections"] );
	gg1.apply( vizGeos["graphVizLightOrigin"] );
	gg2.apply( vizGeos["graphVizLightDir"] );

	for (auto geo : vizGeos) {
		auto mat = VRMaterial::create(geo.first+"_mat");
		mat->setLit(0);
		int r = (geo.first == "graphVizLightOrigin");
		int g = (geo.first == "graphVizLightOrigin" || geo.first == "graphVizLightDir");
		int b = (geo.first == "graphVizLightDir");
		mat->setDiffuse(Color3f(r,g,b));
		mat->setLineWidth(3);
		mat->setPointSize(5);
		geo.second->setMaterial(mat);
	}
}

/** DISABLING VEHICLE GEOMETRIES*/
void VRTrafficSimulation::runWithoutGeometries(){
    isShowingGeometries = false;
}

void VRTrafficSimulation::runWithGeometries(){
    isShowingGeometries = true;
    auto scene = VRScene::getCurrent();
    string strInput = "graphVizVehicMarkers";
    if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();

    string vAnn = "vehicAnn";
    if (scene->getRoot()->find(vAnn)) scene->getRoot()->find(vAnn)->destroy();
}

/** VEHICLE VISION*/
void VRTrafficSimulation::showVehicVision(){
    isShowingVehicleVision = true;
}

void VRTrafficSimulation::hideVehicVision(){
    isShowingVehicleVision = false;
    auto graph = roadNetwork->getGraph();
    auto scene = VRScene::getCurrent();
    string strInput = "graphVizVisionLines";
    if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();
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
    res+= nl + " worldPos: " + toString(v.t->getWorldPose()->pos());
    res+= nl + " isUser: " + toString(v.isUser);
    //res+= nl + " vehiclesight: " + nl +  " INFRONT:" + toString(v.vehiclesight[v.INFRONT]) + " FROMLEFT: " + toString(v.vehiclesight[v.FRONTLEFT]) + " FROMRIGHT:" + toString(v.vehiclesight[v.BEHINDRIGHT]);
    res+= nl + " current_speed: " + toString(v.currentVelocity*3.6);
    res+= nl + " target_speed: " + toString(v.targetVelocity*3.6);
    res+= nl + " nextIntersec: " + toString(v.distanceToNextIntersec);
    res+= nl + " nextSignal: " + toString(v.distanceToNextSignal);
    res+= nl + " roadEdge: " + toString(v.pos.edge);
    res+= nl + " roadPos: " + toString(v.pos.pos);

    return res;
}

string VRTrafficSimulation::getEdgeData(int ID){
    string res = "";
    string nextEdges = "next Edges: ";
    string prevEdges = "prev Edges: ";
    string edgeNeighbors = "Relations: ";
    string edgeLength = "FromTo: ";
    string nl = "\n ";
    auto graph = roadNetwork->getGraph();
    if (!graph->hasEdge(ID)) { return "Road "+toString(ID)+" does not exist in network"; }

    auto& edge = graph->getEdge(ID);

    for (auto nn : graph->getRelations(ID)) { edgeNeighbors +=" " + toString(nn); }
    for (auto nn : graph->getPrevEdges(edge)) { prevEdges +=" " + toString(nn.ID); }
    for (auto nn : graph->getNextEdges(edge)) { nextEdges +=" " + toString(nn.ID); }
    edgeLength += toString(graph->getPosition(edge.from)->pos()) + " " + toString(graph->getPosition(edge.to)->pos());

    res+="Road " + toString(ID) + nl;
    res+=edgeNeighbors + nl;
    res+=prevEdges + nl;
    res+=nextEdges + nl;
    res+=edgeLength;

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
            for (auto nn : graph->getRelations(e.ID)) { edgeNeighbors +=" " + toString(nn); }
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
    returnInfo += nl + "Lanechanging state: " + toString(laneChange);


    CPRINT(returnInfo);
}

bool VRTrafficSimulation::isSeedRoad(int roadID){
    for (auto e : seedRoads){if (roadID==e) return true;}
    return false;
}

void VRTrafficSimulation::setSeedRoad(int debugOverRideSeedRoad){
    this->debugOverRideSeedRoad = debugOverRideSeedRoad;
}

void VRTrafficSimulation::stopVehicle(int ID){
    if ( stopVehicleID < 0 ) stopVehicleID = ID;
    else stopVehicleID = -1;
}
void VRTrafficSimulation::deleteVehicle(int ID){
    if ( deleteVehicleID < 0 ) deleteVehicleID = ID;
    else deleteVehicleID = -1;
}

void VRTrafficSimulation::setSeedRoadVec(vector<int> forceSeedRoads){
    this->forceSeedRoads = forceSeedRoads;
}

void VRTrafficSimulation::toggleLaneChanges(){
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



