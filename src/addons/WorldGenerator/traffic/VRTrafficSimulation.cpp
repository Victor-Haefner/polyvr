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
#include "core/utils/VRTimer.h"
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
#include <chrono>
#include <thread>

#define CPRINT(x) \
VRGuiManager::get()->getConsole( "Console" )->write( string(x)+"\n" );

using namespace OSG;

typedef boost::recursive_mutex::scoped_lock PLock;

template<class T>
void erase(vector<T>& v, const T& t) {
    v.erase(remove(v.begin(), v.end(), t), v.end());
}

template<> string typeName(const OSG::VRTrafficSimulationPtr& t) { return "TrafficSimulation"; }


VRTrafficSimulation::Vehicle::Vehicle(Graph::position p, int type) : pos(p), type(type) {
    //t = VRTransform::create("t");
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

void VRTrafficSimulation::Vehicle::setupSG(VRObjectPtr m, map<string, VRMaterialPtr>& mats) {
    if (t) return;
    t = VRTransform::create("t");
    mesh = m;
    t->addChild(mesh);
    for (auto obj : mesh->getChildren(true, "Geometry")) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        string name = geo->getBaseName();
        if (name == "TurnSignalBL" || name == "turnsignalBL") { geo->makeUnique(); turnsignalsBL.push_back(geo); }
        if (name == "TurnSignalBR" || name == "turnsignalBR") { geo->makeUnique(); turnsignalsBR.push_back(geo); }
        if (name == "TurnSignalFL" || name == "turnsignalFL") { geo->makeUnique(); turnsignalsFL.push_back(geo); }
        if (name == "TurnSignalFR" || name == "turnsignalFR") { geo->makeUnique(); turnsignalsFR.push_back(geo); }
        if (name == "Headlight" || name == "headlight") { geo->makeUnique(); headlights.push_back(geo); }
        if (name == "Backlight" || name == "backlight") { geo->makeUnique(); backlights.push_back(geo); }
    }

    for (auto l : turnsignalsBL) l->setMaterial(mats["carLightOrangeOff"]);
    for (auto l : turnsignalsBR) l->setMaterial(mats["carLightOrangeOff"]);
    for (auto l : turnsignalsFL) l->setMaterial(mats["carLightOrangeOff"]);
    for (auto l : turnsignalsFR) l->setMaterial(mats["carLightOrangeOff"]);
    for (auto l : headlights) l->setMaterial(mats["carLightWhiteOn"]);
    for (auto l : backlights) l->setMaterial(mats["carLightRedOff"]);
}



void VRTrafficSimulation::Vehicle::hide() {
    if (t) t->hide();
}

void VRTrafficSimulation::Vehicle::setDefaults() {

    //PLock lock(mtxV);

    targetVelocity = 50.0/3.6; //try m/s  - km/h
    currentVelocity = targetVelocity;
    float tmp = targetVelocity;
    targetVelocity = targetVelocity*(1.0+0.2*0.01*(rand()%100));
    roadVelocity = targetVelocity;
    distanceToNextIntersec = 10000;
    simPose = Pose::create(Vec3d(0,0,0),Vec3d(0,0,0),Vec3d(0,0,0));

    maxAcceleration = 5;
    maxAcceleration += (targetVelocity - tmp)/5;
    maxDecceleration = 10; //8 dry, 4-5 sand, 1-4 snow
    acceleration = 0.0;
    decceleration = 0.0;
    lastLaneSwitchTS = 0.0;

    pos.pos = 0;
    behavior = 0; //0 = straight, 1 = left, 2 = right
    laneChangeState = 0; //0 = on lane, 1 = leaving lane, -1 = coming onto lane
    roadFrom = -1;
    roadTo = -1;
    nextTurnLane = -1;

    lastMove = Vec3d(0,0,0);
    currentOffset = Vec3d(0,0,0);
    currentdOffset = Vec3d(0,0,0);

    speed = targetVelocity;
    //currentVelocity = 0.0;
    collisionDetected = false;

    signalAhead = false;
    nextSignalState = "000"; //red|organge|green

    Vec3d asdf;
    nextStop = asdf;
}

void VRTrafficSimulation::Vehicle::signalLights(int input, map<string, VRMaterialPtr>& mats) {
    if (input < 0) return;
    if (input == 0) { //straight
        for (auto l : turnsignalsBL) l->setMaterial(mats["carLightOrangeOff"]);
        for (auto l : turnsignalsFL) l->setMaterial(mats["carLightOrangeOff"]);
        for (auto l : turnsignalsBR) l->setMaterial(mats["carLightOrangeOff"]);
        for (auto l : turnsignalsFR) l->setMaterial(mats["carLightOrangeOff"]);
    }
    if (input == 1) { //left
        for (auto l : turnsignalsBL) l->setMaterial(mats["carLightOrangeBlink"]);
        for (auto l : turnsignalsFL) l->setMaterial(mats["carLightOrangeBlink"]);
    }
    if (input == 2) { //right
        for (auto l : turnsignalsBR) l->setMaterial(mats["carLightOrangeBlink"]);
        for (auto l : turnsignalsFR) l->setMaterial(mats["carLightOrangeBlink"]);
    }
    if (input == 3) { //brake
        for (auto l : backlights) l->setMaterial(mats["carLightRedOn"]);
    }
    if (input == 4) { //not braking
        for (auto l : backlights) l->setMaterial(mats["carLightRedOff"]);
    }
}

void VRTrafficSimulation::Vehicle::show() {
    if (t) t->show();
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
    PLock lock(mtx);

    updateCb = VRUpdateCb::create( "traffic", boost::bind(&VRTrafficSimulation::updateSimulation, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);

    auto box = VRGeometry::create("boxCar");
    box->setPrimitive("Box 2 1.5 4 1 1 1");
    addVehicleModel(box);

    auto setupLightMaterial = [&](string name, Color3f c, bool lit) {
        auto l = VRMaterial::create(name);
        l->setLit(lit);
        l->setDiffuse(c);
        lightMaterials[name] = l;
    };

    roadVelocity = 50.0/3.6;

    setupLightMaterial("carLightWhiteOn"  , Color3f(1,1,1), false);
    setupLightMaterial("carLightWhiteOff" , Color3f(0.5,0.5,0.5), true);
    setupLightMaterial("carLightRedOn"    , Color3f(1,0.2,0.2), false);
    setupLightMaterial("carLightRedOff"   , Color3f(0.5,0.1,0.1), true);
    setupLightMaterial("carLightOrangeOn" , Color3f(1,0.7,0.1), false);
    setupLightMaterial("carLightOrangeOff", Color3f(0.5,0.35,0.05), true);
    setupLightMaterial("carLightOrangeBlink", Color3f(1,0.7,0.1), false);

    turnSignalCb = VRUpdateCb::create( "turnSignal", boost::bind(&VRTrafficSimulation::updateTurnSignal, this) );
    VRScene::getCurrent()->addTimeoutFkt(turnSignalCb, 0, 500);

    initiateWorker();
}

VRTrafficSimulation::~VRTrafficSimulation() {}

VRTrafficSimulationPtr VRTrafficSimulation::create() { return VRTrafficSimulationPtr( new VRTrafficSimulation() ); }

void VRTrafficSimulation::setRoadNetwork(VRRoadNetworkPtr rds) {
    roadNetwork = rds;
    roads.clear();
    auto graph = roadNetwork->getGraph();
    for (auto& e : graph->getEdgesCopy()) {
        int eID = e.ID;
        //for (int i = 0; i < graph->getNEdges(); i++) {
        roads[eID] = laneSegment();
        //auto& e = graph->getEdge(i);
        Vec3d p1 = graph->getNode(e.from).p.pos();
        Vec3d p2 = graph->getNode(e.to).p.pos();
        roads[eID].length = (p2-p1).length();
        roads[eID].rID = eID;
        roads[eID].lane = roadNetwork->getLane(eID);
        //roads[eID].road = roadNetwork->getRoad(roads[eID].lane);

        //cout << eID <<" "<< roads[eID].lane->getName() << endl;
        //auto interE = roadNetwork->getIntersection(roads[eID].lane->getEntity("nextIntersection"));
        //if (interE) cout << "   " << interE->getName() << endl;
        //cout << roads[eID].road->getName() << endl;
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

void VRTrafficSimulation::initiateWorker() {
    auto scene = VRScene::getCurrent();
    worker = VRThreadCb::create( "traffic thread", boost::bind(&VRTrafficSimulation::trafficSimThread, this, _1) );
    scene->initThread(worker, "traffic thread", true, 1);
    cout << "VRTrafficSimulation::initiateWorker" << endl;
}


template<class T>
T randomChoice(vector<T> vec) {
    if (vec.size() == 0) return 0;
    auto res = vec[ round((float(random())/RAND_MAX) * (vec.size()-1)) ];
    return res;
}

void VRTrafficSimulation::trafficSimThread(VRThreadWeakPtr tw) {
    if (auto t = tw.lock()) if (t->control_flag == false) return;

    this_thread::sleep_for(chrono::microseconds(500));
    PLock lock(mtx);

    if (!roadNetwork) return;
    auto g = roadNetwork->getGraph();
    auto space = Octree::create(2,10,"trafficSimThread");
    map<int, vector<pair<int, int>>> toChangeRoad;
    map<int, int> toChangeLane;
    map<int, map<int, int>> visionVec;
    float userRadius = 300; // x meter radius around users

    auto clearVecs =[&](){
        toBeMadeInvisVehicles.clear();
        toBeTransformedVehicles.clear();
        visionVecSaved.clear();
    };

    auto addTHVehicle = [&](int roadID, float density, int type) {
        if (maxUnits > 0 && numUnits >= maxUnits) return;

        //roadcheck whether vehicle on seedroad in front of vehicle
        auto& road = roads[roadID];
        auto e = g->getEdge(roadID);
        int n1 = e.from;
        int n2 = e.to;
        float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
        float dis = 100000.0;
        if (road.vehicleIDs.size()>0) {
            //auto pos = vehicles[road.lastVehicleID].t->getFrom();
            auto pos = vehicles[road.lastVehicleID].simPose->pos();
            dis = (pos - g->getNode(n1).p.pos()).length();
            //cout << toString(pos)<< " --- "<< toString(g->getNode(n1).p.pos()) << " --- " << toString(dis)<< endl;
        } //TODO: change 5m below to vehicle length, maybe change 1m to safety distance between cars
        //cout << " density " << density << " " << L << " " << road.vehicleIDs.size() << endl;
        if (dis < (3+5.0/density)) return;  // density of 1 means one car per 6 meter!
        if ( density < float(road.vehicleIDs.size() * 6.0) / L ) return;

        numUnits++;
        auto getVehicle = [&]() {
            if (vehiclePool.size() > 0) {
                auto vID = vehiclePool.back();
                vehiclePool.pop_back();
                toBeShownVehicles.push_back(vID);
                return vID;
            } else {
                auto v = Vehicle( Graph::position(roadID, 0.0), type );
                if (v.getID() == -1) {
                    nID++;
                    v.setID(nID);
                    vehicles[nID] =v;
                    toBeAddedVehicles.push_back(nID);
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
        vehicles[v.vID].simPose = roadNetwork->getPosition( Graph::position(roadID, 0.0) );
        vehicles[v.vID].pos = Graph::position(roadID, 0.0);

        road.vehicleIDs[v.getID()] = v.getID();
        road.lastVehicleID = v.getID();
        //cout << ". . .added vehic " << toString(v.getID()) << toString(vehicles[v.vID].simPose) << endl;
    };

    auto fillOctree = [&]() {
        for (auto& road : roads) { // fill octree
            for (auto& ID : road.second.vehicleIDs) {
                auto pos = vehicles[ID.first].simPose->pos();
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

        auto isIntersectionLane = [&](int ID) {
            auto lane = roadNetwork->getLane(ID);
            if (lane->get("turnDirection")) {
                auto turnDir = lane->get("turnDirection")->value;
                if (turnDir.length() > 2) return true;
            }
            return false;
        };

        for (auto user : users) {
            Vec3d p = user.simPose->pos();
            //Vec3d p = getPoseTo(user.simPose)->pos();
            string debug = "";
            for (auto eV : graph->getEdges()) {
                auto& e = eV.second;

                Vec3d ep1 = graph->getNode(e.from).p.pos();
                Vec3d ep2 = graph->getNode(e.to  ).p.pos();
                float D1 = (ep1-p).length();
                float D2 = (ep2-p).length();

                if (D1 > userRadius && D2 > userRadius) continue; // outside
                if (debugOverRideSeedRoad<0 && graph->getPrevEdges(e).size() == 0  && !isPedestrian(e.ID) && !isParkingLane(e.ID) && !isIntersectionLane(e.ID)) { // roads that start out of "nowhere"
                    newSeedRoads.push_back( e.ID );
                    continue;
                }
                ///TODO: look into radius
                if ( debugOverRideSeedRoad > -2 && debugOverRideSeedRoad < 0 && (D1 > userRadius*0.5 || D2 > userRadius*0.5) && !isPedestrian(e.ID) && !isIntersectionLane(e.ID) ) newSeedRoads.push_back( e.ID ); // on edge
                newNearRoads.push_back( e.ID ); // inside or on edge
            }
        }

        if (debugOverRideSeedRoad!=-1) newSeedRoads.push_back( debugOverRideSeedRoad );

        for (auto roadID : makeDiff(nearRoads, newNearRoads)) {
            auto& road = roads[roadID];
            //for (auto v : road.vehicles) { v.destroy(); numUnits--; }
            for (auto ID : road.vehicleIDs) { toBeHiddenVehicles.push_back(ID.first); vehiclePool.push_front(ID.first); numUnits--; }
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

        int rSeed  = int(abs(float(rand())/float(RAND_MAX) * seedRoads.size()));
        if (seedRoads.size()>0) {
            auto roadID = seedRoads[rSeed];
            auto& road = roads[roadID];
            addTHVehicle(roadID, road.density, 1);
        }
    };

    auto propagateVehicle = [&](Vehicle& vehicle, float d, int intention) {
        auto& gp = vehicle.pos;
        gp.pos += d;

        if (!isTimeForward && gp.pos < 0) { toChangeRoad[gp.edge].push_back( make_pair(vehicle.vID, -1) );}
        if (gp.pos > 1) {
            gp.pos -= 1;
            int road1ID = gp.edge;
            auto& edge = g->getEdge(gp.edge);
            auto nextEdges = g->getNextEdges(edge);

            if (nextEdges.size() > 1) {
                gp.edge = randomChoice(nextEdges).ID;
                for (auto e : nextEdges) {
                    if (e.ID == vehicle.nextTurnLane) { gp.edge = vehicle.nextTurnLane; }
                }
                auto& road = roads[gp.edge];
                if (road.macro) {
                    toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                }
                else {
                    toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                    if (vehicle.laneChangeState != 0) {
                        vehicle.roadTo = g->getNextEdges(g->getEdge(vehicle.roadTo))[0].ID;
                        vehicle.roadFrom = gp.edge;
                    }
                    gp.pos = gp.pos * roads[road1ID].length/roads[gp.edge].length;
                }
                //cout << toString(gp.edge) << endl;
            }
            if (nextEdges.size() == 1) {
                gp.edge = nextEdges[0].ID;
                auto& road = roads[gp.edge];
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                gp.pos = gp.pos * roads[road1ID].length/roads[gp.edge].length;
                if (vehicle.laneChangeState != 0) {
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
        }
        else {
            //auto& edge = g->getEdge(gp.edge);
            //auto nextEdges = g->getNextEdges(edge);
            //if (nextEdges.size() == 0) changeLane(vehicle.vID,1);
        }

        Vec3d offset = Vec3d(0,0,0);
        Vec3d doffset = Vec3d(0,0,0);
        Vec3d dir = vehicle.simPose->dir(); //vehicle.t->getPose()->dir();
        Vec3d up = vehicle.simPose->up(); //vehicle.t->getPose()->up();
        Vec3d left = up.cross(dir);
        Vec3d right = -up.cross(dir);
        //cout << toString(left) << endl;
        float vS = float(vehicle.laneChangeState);
        if (vS == 0) vehicle.currentOffset = Vec3d(0,0,0);
        if (vS == 0) vehicle.currentdOffset = Vec3d(0,0,0);

        float offsetVel = 0.006;
        float doffsetVel = 0.0006;
        //if ( vehicle.currentVelocity*3.6 < 25 ) { offsetVel = 0.008; doffsetVel = 0.0015*2.0; }
        if (intention==vehicle.STRAIGHT) { offset = vehicle.currentOffset + Vec3d(0,0,0); doffset = vehicle.currentdOffset + Vec3d(0,0,0); }
        if (intention==vehicle.SWITCHLEFT && vehicle.indicatorTS > 2 ) { offset = vehicle.currentOffset + left*offsetVel; doffset = vehicle.currentdOffset + left*vS*doffsetVel; }
        if (intention==vehicle.SWITCHRIGHT && vehicle.indicatorTS > 2 ) { offset = vehicle.currentOffset + right*offsetVel; doffset = vehicle.currentdOffset + -left*vS*doffsetVel; }
        //if (intention != 0) { cout << offsetVel << endl; cout << d << endl; }
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
            vehicle.laneChangeState = -1;
            offset = -offset;
            //cout << "trafficsim changing state " << toString(vehicle.vID) << " " << toString(dirOffset) <<endl;
        }
        if (dirOffset < 0.1 && vS<-0.5) {
            vehicle.signaling.push_back(0);
            vehicle.roadFrom = -1;
            vehicle.roadTo = -1;
            vehicle.laneChangeState = 0;
            vehicle.behavior = 0;
            vehicle.nextTurnLane = -1;
            offset = Vec3d(0,0,0);
            doffset = Vec3d(0,0,0);
            //cout << "changed lane" << endl;
        }
        auto p = roadNetwork->getPosition(vehicle.pos);
        //cout << "propagated vehicle pos " <<toString(p) << endl;
        if (offset.length()>20) offset = Vec3d(0,0,0); //DEBUGGING FAIL SAFE
        vehicle.lastMove = p->pos() + offset - vehicle.simPose->pos();
        p->setPos(p->pos()+offset);
        //doffset = Vec3d(0,0,0);
        p->setDir(p->dir()+doffset);
        vehicle.simPose = p;
        //cout << toString(vehicle.vID) << " propagated " << toString(vehicle.simPose) << endl;
        vehicle.lastMoveTS = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
        vehicle.currentOffset = offset;
        vehicle.currentdOffset = doffset;
        //cout << "Vehicle " << vehicle.vehicleID << " " << p->pos() << " " << vehicle.pos.edge << " " << vehicle.pos.pos << endl;
    };

    ///=====PERCEPTION==============================================================================================================================
    auto computePerception = [&](Vehicle& vehicle) {
        auto relativePosition  = [&](int ID, PosePtr p1, PosePtr p2, float disToM, Vec3d lastMove) -> int {
            auto& vehic = vehicles[ID];
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

            if ( d > 0 && disToM < vehic.width/2 ) return INFRONT; // in front, in range, in corridor
            if ( d < 0 && disToM < vehic.width/2 ) return BEHIND; // in behind, in range, in corridor
            if ( d > 0 && left > 1 && left < 5) return FRONTLEFT; // in front, in range, left of corridor
            if ( d > 0 && left <-1 && left >-5) return FRONTRIGHT; // in front, in range, right of corridor
            if ( d < 0 && left > 1 && left < 5) return BEHINDLEFT; // in behind, in range, left of corridor
            if ( d < 0 && left <-1 && left >-5) return BEHINDRIGHT; // in behind, in range, right of corridor
            return -1;
        };

        auto calcFramePoints = [&](Vehicle& vehic) {
        //calculation of 4 points at the edges of vehicle
            //if (vehic.lastFPTS == ) return;
            auto p = vehic.simPose; //t->getPose();
            if (vehic.isUser) {
                p->setPos(p->pos() - globalOffset);
            }
            auto dir = p->dir();
            dir.normalize();
            auto left = p->up().cross(dir);
            auto LH = dir*0.5*vehic.length; //half of vehicle length
            auto WH = left*0.5*vehic.width;   //half of vehicle width
            vehic.vehicleFPs[0] = p->pos() + LH + WH;
            vehic.vehicleFPs[1] = p->pos() + LH - WH;
            vehic.vehicleFPs[2] = p->pos() - LH + WH;
            vehic.vehicleFPs[3] = p->pos() - LH - WH;
            vehic.vehicleFPs[4] = p->pos();
            //vehic.lastFPTS = VRGlobals::CURRENT_FRAME;
        };

        auto calcDisToFP = [&](Vehicle& v1, Vehicle& v2) {
        //simple way to calculate the distance between vehicle1-FPs and vehicle2-middle track
        ///TODO: make B-curve later AGRAJAG
            float res = 5000.0;
            auto dir = v1.simPose->dir(); //t->getPose()->dir();
            for (auto p : v2.vehicleFPs) {
                //float cc = abs((p.second - v1.t->getPose()->pos()).dot(dir.cross(v1.t->getPose()->up())));
                float cc = abs((p.second - v1.simPose->pos()).dot(dir.cross(v1.simPose->up())));
                if (res > cc) res = cc;
            }
            return res;
        };

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

        //========================

        vehicle.vehiclesight.clear();
        if (isSimRunning){
            vehicle.vehiclesightFar.clear();
            vehicle.vehiclesightFarID.clear();
        }

        float safetyDis = vehicle.currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + 6;
        float sightRadius = safetyDis + 8;

        //auto pose = vehicle.t->getPose();
        auto pose = vehicle.simPose;
        auto resFar = space->radiusSearch(pose->pos(), sightRadius);

        for (auto vv : resFar) {
        //check vehicles in radiusSearch
            auto v = (Vehicle*)vv;
            if (!v) continue;
            //if (!v->t) continue;
            //auto p = v->t->getPose();
            auto p = v->simPose;
            auto D = (pose->pos() - p->pos()).length();

            calcFramePoints(vehicles[v->vID]);
            float diss = calcDisToFP(vehicle,vehicles[v->vID]); //distance to middle line of vehicle
            int farP = relativePosition(vehicle.vID, pose, p, diss, vehicle.lastMove);
            setSight(farP,D,v->vID);
            //if (vehicle.currentVelocity > 6 && D <  0.1) vehicle.collisionDetected = true;
        }
        bool tmpUser = false;
        float disDis = 800;
        for (auto& v : users) {
            //if (!v.t) continue;
            //auto vP = v.t->getPoseTo( this->getWorldPose() );
            //auto p = v.t->getWorldPose();
            auto p = v.simPose;
            p->setPos(p->pos() - globalOffset);
            auto simpleDis = (pose->pos() - p->pos()).length();
            if (simpleDis < disDis) disDis = simpleDis;
            if (simpleDis > safetyDis + 10) continue;
            calcFramePoints(v);
            float diss = calcDisToFP(vehicle,v);
            int farP = relativePosition(vehicle.vID, pose, p, diss, vehicle.lastMove);
            setSight(farP,simpleDis,v.vID);
        }
        if (disDis < 200) {
            vehicle.shouldBeVisible = true; toBeTransformedVehicles.push_back(vehicle.vID);
        }
        else vehicle.shouldBeVisible = false;
        if (!vehicle.shouldBeVisible) toBeMadeInvisVehicles.push_back(vehicle.vID);
        //if (!vehicle.isVisible && vehicle.shouldBeVisible) toBeShownVehicles.push_back(vehicle.vID);

        visionVec[vehicle.vID] = vehicle.vehiclesightFarID;

        //========================

        VRTrafficLightPtr nextSignalE;
        Vec3d nextSignalP;

        function<bool (VREntityPtr, int, Vec3d)> recSearch =[&](VREntityPtr newLane, int eID, Vec3d posV) {
        //searches for next intersection in graph, also searches for trafficSignals at intersection
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
                        return recSearch (nextLane, nextID, posV);
                    }
                    else { vehicle.nextTurnLane = -1; vehicle.signalAhead = false; return false; }
                }
                if (type == "intersection") {
                    nextSignalE = interE->getTrafficLight(newLane);
                    auto g = roadNetwork->getGraph();
                    auto& edge = g->getEdge(eID);
                    vehicle.nextStop = g->getNode(edge.to).p.pos();
                    vehicle.distanceToNextStop = (vehicle.nextStop - posV).dot(vehicle.simPose->dir());
                    vehicle.distanceToNextIntersec = (pNode - posV).dot(vehicle.simPose->dir());
                    vehicle.nextIntersection = pNode;

                    ///--------------TURN OPTION DETECTION
                    //trying to set up turn decision at next intersection before actually reaching/crossing intersection
                    auto nextEdges = g->getNextEdges(edge);
                    if (nextEdges.size() > 0) {
                    //see if left, right turn or straight
                        vector<int> res;
                        bool tmpchecknext = false;
                        for (auto e : nextEdges) {
                            res.push_back(e.ID);
                            if (e.ID == vehicle.nextTurnLane) tmpchecknext = true;
                            //making sure nextTurnLane get's reset if vehicle crossed intersection
                        }
                        if (!tmpchecknext) vehicle.nextTurnLane = -1;
                        vehicle.nextLanesCoices = res;
                    }

                    ///--------------LEFT RIGHT FRONT DETECTION
                    //seeing if incoming traffic at next intersection coming from right/left/front
                    auto inLanes = interE->getInLanes();
                    for (auto l : inLanes) {
                        //get vehicles
                        int lID = roadNetwork->getLaneID(l);
                        auto ls = roads[lID];
                        for (auto IDpair : ls.vehicleIDs) {
                            auto v = vehicles[IDpair.second];
                            auto p = v.simPose; //t->getPose();
                            auto D = (pose->pos() - p->pos()).length();
                            if ( D > 1.5*sightRadius && vehicle.turnAhead != 1 || D > 35 ) continue;
                            auto dir = p->dir();
                            auto vDir = vehicle.simPose->dir(); //t->getPose()->dir();
                            auto left = Vec3d(0,1,0).cross(vDir);
                            if (dir.dot(left)>0.7) vehicle.incTrafficRight = true; //cout << "incoming right" << endl;
                            if (dir.dot(left)<-0.7) vehicle.incTrafficLeft = true; //cout << "incoming left" << endl;
                            if (dir.dot(vDir)<-0.7) {
                                if (v.turnAhead == 1) continue;
                                vehicle.incTrafficFront = true;
                                vehicle.frontVehicLastMove = v.lastMoveTS;
                            }
                       }
                    }

                    ///--------------SIGNAL DETECTION
                    if (nextSignalE) {
                        nextSignalP = nextSignalE->getFrom();
                        //vehicle.distanceToNextSignal  = (nextSignalP - posV).dot(vehicle.t->getPose()->dir());
                        vehicle.distanceToNextSignal  = (nextSignalP - posV).dot(vehicle.simPose->dir()); //t->getPose()->dir());
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
        vehicle.distanceToNextSignal = 10000;
        vehicle.distanceToNextStop = 10000;
        vehicle.incTrafficRight = false;
        vehicle.incTrafficLeft = false;
        vehicle.incTrafficFront = false;
        //recSearch(laneE,vehicle.pos.edge,vehicle.t->getPose()->pos());
        recSearch(laneE,vehicle.pos.edge,vehicle.simPose->pos());
        if (vehicle.nextTurnLane != -1) {
            auto nextRoad = roadNetwork->getLane(vehicle.nextTurnLane);
            if (nextRoad->get("turnDirection")) {
                auto turnDir = nextRoad->get("turnDirection")->value;
                if (turnDir == "straight") { vehicle.turnAhead = 0; }
                if (turnDir == "left") { vehicle.turnAhead = 1; }
                if (turnDir == "right") { vehicle.turnAhead = 2; }
            }
        } else { vehicle.turnAhead = -1; }
    };

    auto computeRoutingDecision = [&](Vehicle& vehicle) {
        //right now random, IDEA: get routing integrated here
        if (vehicle.nextTurnLane != -1) return;
        if (vehicle.nextLanesCoices.size() == 1 ) {
            vehicle.nextTurnLane = vehicle.nextLanesCoices[0];
            return;
        }
        if (vehicle.nextLanesCoices.size() > 1) {
            vehicle.nextTurnLane = vehicle.nextLanesCoices[round((float(random())/RAND_MAX) * (vehicle.nextLanesCoices.size()-1))];
            return;
        }
    };

    auto propagateVehicles = [&]() {
        int N = 0;
        float current = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
        //cout << current << endl;
        deltaT = current - lastT;
        if (deltaT == 0) {
            this_thread::sleep_for(chrono::microseconds(1000));
            float current = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
            deltaT = current - lastT;
        }
        lastT = current;
        for (auto& road : roads) {
            for (auto& ID : road.second.vehicleIDs) {
                auto& vehicle = vehicles[ID.first];
                //if (!vehicle.t) continue;

                computePerception(vehicle);
                computeRoutingDecision(vehicle);
                //computeAction(vehicle);

                float d = vehicle.currentVelocity;
                float safetyDis = vehicle.currentVelocity*3.6 * environmentFactor * roadFactor / 4.0 + vehicle.length + 1;
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
                float sinceLastLS = float(glutGet(GLUT_ELAPSED_TIME)*0.001) - vehicle.lastLaneSwitchTS;

                float intersectionWidth = vehicle.distanceToNextIntersec - vehicle.distanceToNextStop;
                bool signalBlock = vehicle.nextSignalState=="100";
                bool signalTransit = vehicle.nextSignalState=="010";
                bool interBlock = (vehicle.distanceToNextStop < 60 && vehicle.distanceToNextIntersec > intersectionWidth);
                bool vehicBlock = false;
                bool holdAtIntersec = false;
                bool inIntersec = vehicle.distanceToNextIntersec < intersectionWidth;
                //if (nextSignal != "000") cout << toString(nextSignal) << endl;

                vehicle.targetVelocity = roadVelocity;
                if ( vehicle.distanceToNextStop < 15 && vehicle.turnAhead>0 && !inIntersec ) vehicle.targetVelocity = 30/3.6;
                if ( vehicle.distanceToNextStop < 5 && vehicle.turnAhead>0 && !inIntersec) vehicle.targetVelocity = 23/3.6;
                if ( vehicle.distanceToNextStop > 15 || inIntersec) vehicle.targetVelocity = 50/3.6;

                auto inFront = [&]() { return vehicle.vehiclesightFarID.count(INFRONT); };
                auto comingLeft = [&]() { return vehicle.vehiclesightFarID.count(FRONTLEFT); };
                auto comingRight = [&]() { return vehicle.vehiclesightFarID.count(FRONTRIGHT); };

                ///Velocity control functions
                auto accelerate = [&](float f) {
                    d = vehicle.currentVelocity + accFactor*deltaT*f;
                    vehicle.signaling.push_back(4);
                };
                auto decelerate = [&](float f) {
                    d = vehicle.currentVelocity + decFactor*deltaT*f;
                    vehicle.signaling.push_back(3);
                };
                auto holdVelocity = [&]() {
                    d = vehicle.currentVelocity;
                    vehicle.signaling.push_back(4);
                };

                auto behave = [&]() {
                ///LOGIC
                    float nextSignalDistance = vehicle.distanceToNextSignal;
                    float nextIntersection = vehicle.distanceToNextIntersec;
                    float nextStopDistance = vehicle.distanceToNextStop;
                    bool signalAhead = vehicle.signalAhead;
                    bool turnAhead = vehicle.turnAhead>0;
                    turnAhead = true;

                    ///INDICATORS
                    if (nextIntersection > 35 && nextStopDistance > 30) vehicle.signaling.push_back(0);
                    if (vehicle.turnAhead == 1 && nextIntersection < 35 && nextStopDistance < 30) vehicle.signaling.push_back(1); //left
                    if (vehicle.turnAhead == 2 && nextIntersection < 35 && nextStopDistance < 30) vehicle.signaling.push_back(2); //right
                    /*
                    ///VELOCITY CONTROL
                    if ( vbeh == vehicle.STRAIGHT ) {
                        if ( !inFront() ) {
                        //no vehicle ahead
                            ///APPROACHING INTERSECTION
                            if ( signalAhead && nextStopDistance < safetyDis + 20 && signalBlock ) { driveToIntersec(); return; }
                            if ( signalAhead && nextStopDistance < safetyDis + 3 && signalTransit ) { accelerate(1); return; }
                            if ( signalAhead && nextStopDistance > safetyDis + 3 && signalTransit ) { driveToIntersec(); return; }
                            //if ( !signalAhead && nextSignalDistance < safetyDis + 6 && signalBlock ) { driveToIntersec(); return; }
                            //if ( nextSignalDistance < safetyDis + 6 ) { driveToIntersec(); return; }

                            //if ( vehicle.turnAtIntersec && ( comingRight() || comingLeft() ) && !nextVehicleTurner() ) { decelerate(); return; }
                            if ( sinceLastLS > 10 && nextIntersection > 20 && checkR(vehicle.vID) && vehicle.currentVelocity > 5/3.6 ) { toChangeLane[vehicle.vID] = 2; }
                            //check if road is ending/intersections etc - possible breaking
                            if ( vehicle.currentVelocity < vehicle.targetVelocity*0.95 ) { accelerate(1); return; }
                            //targetVelocity not reached //accelerate
                            if ( vehicle.currentVelocity > vehicle.targetVelocity*1.05 ) { decelerate(1); return; }
                            //targetVelocity overreached //decelerate
                            if ( vehicle.currentVelocity > vehicle.targetVelocity*0.95 && vehicle.currentVelocity < vehicle.targetVelocity*1.05 ) { holdVelocity(); return; }
                            //targetVelocity reached //proceed
                        }
                        if ( inFront() ) {
                        //vehicle ahead?
                            if ( vehicle.currentVelocity == 0 && signalAhead && nextStopDistance < 4 && signalBlock ) { return; }
                            if ( vehicle.currentVelocity > 0 && signalAhead && nextStopDistance < 4 && signalBlock && !inIntersec ) { driveToIntersec(); return; }
                            if ( vehicle.currentVelocity > 0 && signalAhead && nextStopDistance < 4 && signalBlock &&  inIntersec ) { decelerate(1); return; }
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            //if ( vehicle.turnAtIntersec && !nextVehicleTurner() ) { decelerate(); return; }
                            if ( disToFrontV - nextMove < safetyDis ) { decelerate(1); return; }
                            //if ( nextIntersection < safetyDis + 0.2 ) { decelerate(); return; }
                            if ( vehicles[frontID].currentVelocity > vehicle.currentVelocity && vehicle.currentVelocity < vehicle.targetVelocity ) {
                            //vehicle ahead faster, and targetVelocity not reached
                                if ( disToFrontV - nextMoveAcc > safetyDis + 2 ) { accelerate(1); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(1); return; }
                                //break/decelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis + 2 && disToFrontV - nextMoveAcc > safetyDis  ) { holdVelocity(); return; }
                                //hold velocity
                            }
                            if ( vehicles[frontID].currentVelocity <= vehicle.currentVelocity && vehicle.currentVelocity < vehicle.targetVelocity ) {
                            //vehicle ahead slower
                                if ( sinceLastLS > 10 && nextIntersection > 30 && checkL(vehicle.vID) && vehicle.currentVelocity > 20/3.6 ) { toChangeLane[vehicle.vID] = 1; }
                                //check if lane switch possible
                                if ( disToFrontV - nextMoveAcc > safetyDis + 2 ) { accelerate(1); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(1); return; }
                                //break/decelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis + 2 && disToFrontV - nextMoveAcc > safetyDis  ) { holdVelocity(); return; }
                                //hold velocity
                            }
                            if ( vehicle.currentVelocity >= vehicle.targetVelocity ) {
                            //vehicle ahead slower
                                if ( sinceLastLS > 10 && nextIntersection > 30 && checkR(vehicle.vID) && vehicle.currentVelocity > 20/3.6 ) { toChangeLane[vehicle.vID] = 2; }
                                //check if lane switch possible
                                if ( disToFrontV - nextMove > safetyDis && vehicle.currentVelocity == vehicle.targetVelocity ) { holdVelocity(); return; }
                                //accelerate
                                if ( disToFrontV - nextMoveAcc < safetyDis ) { decelerate(1); return; }
                                //break/decelerate
                            }
                        }
                    }*/

                    auto checkLaneSwitch =[&]() {
                        if (sinceLastLS < 10 || nextStopDistance < 35 || vehicle.currentVelocity < 15/3.6 ) return;
                        if ( inFront() ) {
                            if ( checkL(vehicle.vID) ) toChangeLane[vehicle.vID] = 1;
                        }
                        else {
                            if ( checkR(vehicle.vID) ) toChangeLane[vehicle.vID] = 2;
                        }
                    };

                    auto checkAcceleration =[&]() {
                        bool safeTravel = nextStopDistance - nextMoveAcc > safetyDis;
                        if ( vehicle.currentVelocity > vehicle.targetVelocity*0.95 ) return false;
                        if (  signalAhead && signalBlock && !safeTravel ) return false; //red light
                        if (  signalAhead && signalTransit && nextStopDistance - nextMoveAcc < safetyDis + 1 && !inIntersec ) return false; //orange light
                        if ( !signalAhead && vehicle.incTrafficRight && vehicle.turnAhead != 2 && !safeTravel ) return false;
                        if ( vehicle.turnAhead == 1 && vehicle.incTrafficFront && nextStopDistance < 5  ) return false;
                        if ( inFront() ) {
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            if ( disToFrontV - nextMoveAcc < safetyDis + 3 ) return false;
                        }
                        return true;
                    };

                    auto checkHoldVelocity =[&]() {
                        bool safeTravel = nextStopDistance - nextMove > safetyDis - 2;
                        if ( vehicle.currentVelocity > vehicle.targetVelocity ) return false;
                        if (  signalAhead && signalBlock && !safeTravel ) return false;
                        if (  signalAhead && signalTransit && !safeTravel && !inIntersec ) return false; //orange light
                        if ( !signalAhead && vehicle.incTrafficRight && vehicle.turnAhead != 2 && !safeTravel ) return false;
                        if ( vehicle.turnAhead == 1 && vehicle.incTrafficFront && nextStopDistance < 1.5  ) return false;
                        if ( inFront() ) {
                            int frontID = vehicle.vehiclesightFarID[INFRONT];
                            float disToFrontV = vehicle.vehiclesightFar[INFRONT];
                            if ( disToFrontV - nextMove < safetyDis + 1 ) return false;
                        }
                        return true;
                    };

                    auto checkDeceleration =[&]() {
                        return true;
                    };

                    checkLaneSwitch();
                    if ( checkAcceleration() ) { accelerate(1); return; }
                    if ( checkHoldVelocity() ) { holdVelocity(); return; }
                    if ( checkDeceleration() ) { decelerate(1); return; }
                };
                behave();
                vehicle.currentVelocity = d;
                d *= deltaT/road.second.length;
                if (d<0.0000003 && vehicle.pos.pos > 0.1 && signalBlock) d = 0;
                //if (nextSignalDistance > 0.5 && nextSignalDistance < 5 && nextSignalState=="100") d = 0; //hack
                if (!isSimRunning) d = 0;
                if (stopVehicleID == ID.first) d = 0;
                if (isSimRunning && d<=0 && vbeh != vehicle.REVERSE) { d = 0; vehicle.currentVelocity = d; }
                if (vehicle.collisionDetected) d = 0;
                if (d!=0 && speedMultiplier!=1.0) d*=speedMultiplier;
                if (!isTimeForward) d = -d;
                if (d!=0) propagateVehicle(vehicle, d, vbeh);

                if (isSimRunning && float(glutGet(GLUT_ELAPSED_TIME)*0.001) - vehicle.lastMoveTS > killswitch1 && !interBlock && !vehicBlock) { // && !interBlock && stopVehicleID != ID.first) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle.vID, -1) ); ///------killswitch if vehicle get's stuck
                }
                if (isSimRunning && float(glutGet(GLUT_ELAPSED_TIME)*0.001) - vehicle.lastMoveTS > killswitch2) { // && !interBlock && stopVehicleID != ID.first) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle.vID, -1) ); ///------killswitch if vehicle get's stuck
                }
                if (!isSimRunning) vehicle.lastMoveTS = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
                N++; // count vehicles!
            }
        }

        visionVecSaved = visionVec;
        //cout << " .. propagated " << toString(N) << " vehicles" << endl;
    };

    auto resolveRoadChanges = [&]() {
        for (auto r : toChangeRoad) {
            auto& road = roads[r.first];
            for (auto v : r.second) {
                road.vehicleIDs.erase(v.first);
                if (v.second == -1) {
                    auto& vehicle = vehicles[v.first];
                    toBeHiddenVehicles.push_back(vehicle.vID);
                    auto& gp = vehicle.pos;
                    auto p = roadNetwork->getPosition(vehicle.pos);
                    Vec3d offset = Vec3d(0,-30,0);
                    p->setPos(p->pos()+offset);
                    vehicle.simPose = p;
                    gp.pos = 0;
                    vehicle.setDefaults();
                    vehiclePool.push_front(v.first);
                    vehicle.signaling.push_back(0);
                    toBeMadeInvisVehicles.push_back(vehicle.vID);
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

    clearVecs();
    updateSimulationArea();
    fillOctree();
    propagateVehicles();
    //resolveCollisions();
    //updateDensityVisual();
    resolveRoadChanges();
    resolveLaneChanges();

    //cout << "-------hello, this is traffic sim thread " << deltaT << endl;
}

void VRTrafficSimulation::updateSimulation() {
    if (!roadNetwork) return;

    PLock lock(mtx);
    vector<int> N(4,0);
    auto addTransforms = [&]() {
        if (toBeAddedVehicles.size() == 0) return;
        for (auto each : toBeAddedVehicles) {
            Vehicle& v = vehicles[each];
            v.setupSG(models[v.type]->duplicate(), lightMaterials);
            addChild(v.t);
            N[0]++;
        }
        toBeAddedVehicles.clear();
    };

    auto hideTransforms = [&]() {
        if (toBeHiddenVehicles.size() == 0) return;
        for (auto each : toBeHiddenVehicles) {
            Vehicle& v = vehicles[each];
            v.hide();
            N[1]++;
        }
        toBeHiddenVehicles.clear();
    };

    auto showTransforms = [&]() {
        if (toBeShownVehicles.size() == 0) return;
        float start = glutGet(GLUT_ELAPSED_TIME);
        for (auto each : toBeShownVehicles) {
            Vehicle& v = vehicles[each];
            v.show();
            N[2]++;
        }
        toBeShownVehicles.clear();
        float stop = glutGet(GLUT_ELAPSED_TIME);
        float delt = stop-start;
        //if (N[2] > 0 && delt>0) cout << "showTransforms " << N[2] << " "  << delt << endl;
    };

    auto makeInvisTransforms = [&]() {
        if (toBeMadeInvisVehicles.size() == 0) return;
        for (auto each : toBeMadeInvisVehicles) {
            Vehicle& v = vehicles[each];
            v.t->setVisible(false);
            //v.isVisible = false;
        }
        toBeMadeInvisVehicles.clear();
    };

    auto updateTransforms = [&]() {
        for (auto& u : users) {
            if (!u.t)  continue;
            u.simPose = u.t->getWorldPose();
            //N[3]++;
        }
        if (toBeTransformedVehicles.size() == 0) return;
        for (auto each : toBeTransformedVehicles) {
            auto& v = vehicles[each];
            if (v.t) {
                auto pose = v.simPose;
                v.t->setPose(v.simPose);
                v.t->setVisible(true);
                //v.isVisible = true;
                for (auto i : v.signaling) v.signalLights(i, lightMaterials);
                v.signaling.clear();
                //N[3]++;
            }
        }
        toBeTransformedVehicles.clear();
    };

    auto updateVisuals = [&](){
        if (isShowingGraph) updateGraph();
        if (isShowingVehicleVision) updateVehicVision();
        if (isShowingMarkers) updateVehicIntersecs();
    };

    float starter = glutGet(GLUT_ELAPSED_TIME);
    addTransforms();
    hideTransforms();
    showTransforms();
    makeInvisTransforms();
    updateTransforms();
    updateVisuals();
    float stopper = glutGet(GLUT_ELAPSED_TIME);
    float delta = stopper-starter;
    //if (delta > 3) cout << "everything " << delta << " - " << N[3] << endl;

    //cout << "lul " << Vec4i(N[0], N[1], N[2], N[3]) << endl;
}

void VRTrafficSimulation::updateTurnSignal() {
    bool l = !lightMaterials["carLightOrangeBlink"]->isLit();
    if (l) lightMaterials["carLightOrangeBlink"]->setDiffuse(Color3f(0.5,0.35,0.05));
    else   lightMaterials["carLightOrangeBlink"]->setDiffuse(Color3f(1,0.7,0.1));
    lightMaterials["carLightOrangeBlink"]->setLit(l);
}

VRTransformPtr VRTrafficSimulation::getUser() { return users[0].t; }

void VRTrafficSimulation::addUser(VRTransformPtr t) {

    PLock lock(mtx);

    auto v = Vehicle( Graph::position(0, 0.0), 1 );
    nID++;
    v.setID(nID);
    v.isUser = true;
    vehicles[nID] = v;
    users.push_back(v);
    users[users.size()-1].t = t;
    cout << "VRTrafficSimulation::addUser " << nID << endl;
}

void VRTrafficSimulation::setGlobalOffset(Vec3d globalOffset) { this->globalOffset = globalOffset; }

void VRTrafficSimulation::addVehicle(int roadID, float density, int type) {
    /*if (maxUnits > 0 && numUnits >= maxUnits) return;

    PLock lock(mtx);

    //roadcheck whether vehicle on seedroad in front of vehicle
    auto& road = roads[roadID];
    auto g = roadNetwork->getGraph();
    auto e = g->getEdge(roadID);
    int n1 = e.from;
    int n2 = e.to;
    float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
    float dis = 100000.0;
    if (road.vehicleIDs.size()>0) {
        //auto pos = vehicles[road.lastVehicleID].t->getFrom();
        auto pos = vehicles[road.lastVehicleID].simPose->pos();
        dis = (pos - g->getNode(n1).p.pos()).length();
        //cout << toString(pos)<< " --- "<< toString(g->getNode(n1).p.pos()) << " --- " << toString(dis)<< endl;
    } //TODO: change 5m below to vehicle length, maybe change 1m to safety distance between cars
    if (dis < (1+5.0/density)) return;  // density of 1 means one car per 6 meter!

    numUnits++;
    auto getVehicle = [&]() {
        if (vehiclePool.size() > 0) {
            auto v = vehiclePool.back();
            vehiclePool.pop_back();
            vehicles[v.vID].show();
            return v.vID;
        } else {
            auto v = Vehicle( Graph::position(roadID, 0.0), type );

            v.mesh = models[type]->duplicate();
            for (auto obj : v.mesh->getChildren(true, "Geometry")) {
                VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
                string name = geo->getBaseName();
                if (name == "TurnSignalBL" || name == "turnsignalBL") { geo->makeUnique(); v.turnsignalsBL.push_back(geo); }
                if (name == "TurnSignalBR" || name == "turnsignalBR") { geo->makeUnique(); v.turnsignalsBR.push_back(geo); }
                if (name == "TurnSignalFL" || name == "turnsignalFL") { geo->makeUnique(); v.turnsignalsFL.push_back(geo); }
                if (name == "TurnSignalFR" || name == "turnsignalFR") { geo->makeUnique(); v.turnsignalsFR.push_back(geo); }
                if (name == "Headlight" || name == "headlight") { geo->makeUnique(); v.headlights.push_back(geo); }
                if (name == "Backlight" || name == "backlight") { geo->makeUnique(); v.backlights.push_back(geo); }
            }

            //for (auto l : v.turnsignalsBL) l->setMaterial(carLightOrangeOff);
            //for (auto l : v.turnsignalsBR) l->setMaterial(carLightOrangeOff);
            //for (auto l : v.turnsignalsFL) l->setMaterial(carLightOrangeOff);
            /*for (auto l : v.turnsignalsFR) l->setMaterial(carLightOrangeOff);
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
    vehicles[v.vID].simPose = roadNetwork->getPosition( Graph::position(roadID, 0.0) );
    cout << "........trying to add vehicle " << toString(roadNetwork->getPosition( Graph::position(roadID, 0.0) )) << endl;
    cout << "...pp " << toString(vehicles[v.vID].simPose) << endl;

    //if (VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(v.mesh) ) g->makeUnique(); // only for debugging!!
    //v.t->setPickable(true);

    v.t->addChild(v.mesh);
    cout << "step 1" << endl;
    addChild(v.t);
    cout << "step 2" << endl;
    road.vehicleIDs[v.getID()] = v.getID();
    road.lastVehicleID = v.getID();
    cout << ". . .added vehic " << toString(v.getID()) << toString(vehicles[v.vID].simPose) << endl;*/
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

    PLock lock(mtx);

    auto& v = vehicles[ID];
    if (v.behavior != 0) return;
    auto& gp = v.pos;
    auto& edge = roadNetwork->getGraph()->getEdge(gp.edge);
    auto posV = v.pos; //current vehicle position
    auto pos = v.pos; //position on alternative road
    auto poseV = roadNetwork->getPosition(gp);
    auto vDir = v.simPose->dir();
    auto vUp = v.simPose->up();
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

    if ( direction == 1 && check(1) ) { checked = true; v.roadTo = edgeLeft; v.speed += 0.03; v.signaling.push_back(1); }
    if ( direction == 2 && check(2)) { checked = true; v.roadTo = edgeRight; v.speed -= 0.03; v.signaling.push_back(2); }
    if ( checked ){
        v.laneChangeState = 1;
        v.behavior = direction;
        v.roadFrom = gp.edge;
        v.indicatorTS = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
        v.lastLaneSwitchTS = float(glutGet(GLUT_ELAPSED_TIME)*0.001);
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

void VRTrafficSimulation::toggleVisibility() {
    if ( hidden) { this->show(); hidden = false; }
    if (!hidden) { this->hide(); hidden = true; }
}

void VRTrafficSimulation::setSpeedmultiplier(float speedMultiplier) {
    this->speedMultiplier = speedMultiplier;
}

void VRTrafficSimulation::toggleDirection() {
    isTimeForward = !isTimeForward;
}

/** SHOW GRAPH */
void VRTrafficSimulation::toggleGraph(){
    isShowingGraph = !isShowingGraph;
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

/** SHOW INTERSECTION */
void VRTrafficSimulation::toggleIntersections(){
    isShowingIntersecs = !isShowingIntersecs;
    updateIntersectionVis(isShowingIntersecs);
}

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

/** VEHICLE VISION*/
void VRTrafficSimulation::toggleVehicVision(){
    isShowingVehicleVision = !isShowingVehicleVision;
    updateVehicVision();
}

void VRTrafficSimulation::updateVehicVision(){
    auto graph = roadNetwork->getGraph();
    auto scene = VRScene::getCurrent();
    string strInputvL = "graphVizVisionLines";
    if ( scene->getRoot()->find(strInputvL) ) scene->getRoot()->find(strInputvL)->destroy();
    string strInputvM = "graphVizVehicMarkers";
    if ( scene->getRoot()->find(strInputvM) ) scene->getRoot()->find(strInputvM)->destroy();
    string vAnn = "vehicAnn";
    if (scene->getRoot()->find(vAnn)) scene->getRoot()->find(vAnn)->destroy();

    if (!isShowingVehicleVision) return;
    PLock lock(mtx);
    VRGeoData gg0;
    if (visionVecSaved.size()==0) return;
    for (auto vv : visionVecSaved){
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

            auto nPose1 = vehicles[ID1].simPose;
            auto nPose2 = vehicles[ID2].simPose;

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
        auto graphViz = VRGeometry::create(strInputvL);
        graphViz->setPersistency(0);
        scene->getRoot()->addChild(graphViz);

        gg0.apply( graphViz );

        auto mat = VRMaterial::create(strInputvL+"_mat");
        mat->setLit(0);
        mat->setDiffuse(Color3f(1,1,0));
        mat->setLineWidth(3);
        graphViz->setMaterial(mat);
    }
    auto vehicAnn = VRAnnotationEngine::create(vAnn);
    if (isShowingMarkers) return;
    vehicAnn->setPersistency(0);
    vehicAnn->setBillboard(true);
    vehicAnn->setBackground(Color4f(1,1,1,1));
    addChild(vehicAnn);

    VRGeoData gg1;
    if (vehicles.size()==0) return;
    for (auto vv : vehicles){
        auto v = vv.second;
        //if (!v) continue;
        if (!v.t) continue;
        if (v.isUser) continue;
        auto vPose = v.t->getWorldPose();
        auto p = vPose->pos() + Vec3d(0,1.7,0);
        auto p2 = v.t->getPose()->pos() + Vec3d(0,1.7,0);
        int vID1 = gg1.pushVert(p);
        gg1.pushColor(Color3f(1,1,0));
        gg1.pushPoint();
        vehicAnn->set(vID1, p2 + Vec3d(0,0.2,0), "V: "+toString(v.getID()));
    }
    if (gg1.size() > 0) {
        auto graphViz = VRGeometry::create(strInputvM);
        graphViz->setPersistency(0);
        scene->getRoot()->addChild(graphViz);

        gg1.apply( graphViz );

        auto mat1 = VRMaterial::create(strInputvM+"_mat");
        mat1->setLit(0);
        mat1->setDiffuse(Color3f(1,1,0));
        mat1->setLineWidth(3);
        mat1->setPointSize(4);
        graphViz->setMaterial(mat1);
    }
}

/** VEHICLE MARKER*/
void VRTrafficSimulation::toggleVehicMarkers(int i){
    isShowingMarkers = !isShowingMarkers;
    whichVehicleMarkers = i;
    updateVehicIntersecs();
}

void VRTrafficSimulation::updateVehicIntersecs(){
    auto scene = VRScene::getCurrent();
    string strInput = "graphVizVehicMarkers";
    if ( scene->getRoot()->find(strInput) ) scene->getRoot()->find(strInput)->destroy();

    string vAnn = "vehicAnn";
    if (scene->getRoot()->find(vAnn)) scene->getRoot()->find(vAnn)->destroy();
    if (!isShowingMarkers) return;
    PLock lock(mtx);
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
        auto vPose = v.simPose;
        auto p = vPose->pos() + Vec3d(0,1.7,0);
        auto p2 = v.t->getPose()->pos() + Vec3d(0,1.7,0);
        int vID1 = gg0.pushVert(p);
        gg0.pushColor(Color3f(1,1,0));
        gg0.pushPoint();
        vehicAnn->set(vID1, p2 + Vec3d(0,0.2,0), "V: "+toString(v.getID()));
        auto node = v.nextStop;
        if (node.length() > 0) {
            auto po1 = vPose->pos() + Vec3d(0,.5,0);
            auto po2 = node + Vec3d(0,.5,0);
            auto vvID1 = gg0.pushVert(po1);
            Color3f cl = Color3f(0,0,0);
            if (v.turnAhead == 0) cl = Color3f(0,0,0.5);
            if (v.turnAhead == 1) cl = Color3f(1,0,0);
            if (v.turnAhead == 2) cl = Color3f(1,1,0);
            gg0.pushColor(cl);
            auto vvID2 = gg0.pushVert(po2);
            gg0.pushColor(cl);
            gg0.pushLine(vvID1,vvID2);
        }
        /*auto node = v.nextIntersection;
        if (node.length() > 0) {
            if (whichVehicleMarkers == 0 && v.turnAhead != 0) continue;
            if (whichVehicleMarkers == 1 && v.turnAhead != 1) continue;
            if (whichVehicleMarkers == 2 && v.turnAhead != 2) continue;
            auto po1 = vPose->pos() + Vec3d(0,.5,0);
            auto po2 = node + Vec3d(0,.5,0);
            auto vvID1 = gg0.pushVert(po1);
            Color3f cl = Color3f(0,0,0);
            if (v.turnAhead == 0) cl = Color3f(0,0,0.5);
            if (v.turnAhead == 1) cl = Color3f(1,0,0);
            if (v.turnAhead == 2) cl = Color3f(1,1,0);
            gg0.pushColor(cl);
            auto vvID2 = gg0.pushVert(po2);
            gg0.pushColor(cl);
            gg0.pushLine(vvID1,vvID2);
        }*/
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
    res+= nl + " simPos: " + toString(v.simPose->pos());
    res+= nl + " isUser: " + toString(v.isUser);
    //res+= nl + " vehiclesight: " + nl +  " INFRONT:" + toString(v.vehiclesight[v.INFRONT]) + " FROMLEFT: " + toString(v.vehiclesight[v.FRONTLEFT]) + " FROMRIGHT:" + toString(v.vehiclesight[v.BEHINDRIGHT]);
    res+= nl + " current_speed: " + toString(v.currentVelocity*3.6);
    res+= nl + " target_speed: " + toString(v.targetVelocity*3.6);
    res+= nl + " max_Acceleration: " + toString(v.maxAcceleration*3.6);
    res+= nl + " nextIntersec: " + toString(v.distanceToNextIntersec);
    res+= nl + " nextStop: " + toString(v.distanceToNextStop);
    res+= nl + " nextSignal: " + toString(v.distanceToNextSignal);
    res+= nl + " nextSignalB: " + toString(v.signalAhead);
    res+= nl + " nextSignalState: " + v.nextSignalState;
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

void VRTrafficSimulation::setKillswitches(float k1,float k2){
    killswitch1 = k1;
    killswitch2 = k2;
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

    string tmpSeeds = "";
    for (auto roadID : seedRoads) {
        auto& road = roads[roadID];
        //addVehicles(roadID, road.density, 1); // TODO: pass a vehicle type!!
        tmpSeeds += " " + toString(roadID);
    }

    returnInfo += tmpSeeds;
    returnInfo += nl + fit(n1) + "--" + roadInfo;
    returnInfo += nl + fit(n1) + "--" + edgeInfo;
    returnInfo += nl + fit(n2) + "--" + edgeNeighbors;
    returnInfo += nl + fit(n3) + "--" + nodeInfo;
    returnInfo += nl + fit(n4) + "--" + vehiInfo;
    returnInfo += nl + "Lanechanging state: " + toString(laneChange);


    CPRINT(returnInfo);
}

void VRTrafficSimulation::runVehicleDiagnostics(){
    string returnInfo = "";
    string nl = "\n ";
    string vehiInfo = "ALL VEHICLES: ";
    string posEdgeInfo = "Positions Edge: ";
    string posPosInfo = "Positions Pos: ";
    string velInfo = "Velocities: ";
    string tarVelInfo = "Target Velocities: ";
    string maxAccel = "maxAcceleration: ";

    PLock lock(mtx);

    auto fit = [&](int input) {
        string res = "";
        int l1 = toString(input).length();
        for (int i=l1 ; i<4 ; i++) res+=" ";
        return res+toString(input);
    };

    ///get all vehicles
    int n1=0;
    int vis = 0;
    for (auto v : vehicles) {
        vehiInfo += toString(v.second.vID) + " ";
        posEdgeInfo += toString(v.second.pos.edge) + " ";
        posPosInfo += toString(v.second.pos.pos) + " ";
        velInfo += toString(v.second.currentVelocity * 3.6) + " ";
        tarVelInfo += toString(v.second.targetVelocity * 3.6) + " ";
        maxAccel += toString(v.second.maxAcceleration) + " ";
        if (v.second.isVisible) vis ++;
        n1++;
    }

    returnInfo += nl + fit(n1) + "--" + vehiInfo;
    returnInfo += nl + fit(n1) + "--" + posEdgeInfo;
    returnInfo += nl + fit(n1) + "--" + posPosInfo;
    returnInfo += nl + fit(n1) + "--" + velInfo;
    returnInfo += nl + fit(n1) + "--" + tarVelInfo;
    returnInfo += nl + fit(n1) + "--" + maxAccel;
    returnInfo += nl + fit(vis) + "-- visible";

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



