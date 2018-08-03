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

#include <boost/bind.hpp>

#define CPRINT(x) \
VRGuiManager::get()->getConsole( "Console" )->write( x+"\n" );

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
    float userRadius = 150; // x meter radius around users

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
                if (D1 > userRadius*0.5 || D2 > userRadius*0.5) newSeedRoads.push_back( e.ID ); // on edge
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
            cout << seedRoadsString << endl;
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
                if (road.macro) toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                else toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                //cout << toString(gp.edge) << endl;
            }
            if (nextEdges.size() == 1) {
                gp.edge = nextEdges[0].ID;
                auto& road = roads[gp.edge];
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, gp.edge) );
                //cout << "  transit of vehicle: " << vehicle.getID() << " from: " << road1ID << " to: " << gp.edge << endl;
            }
            if (nextEdges.size() == 0) {
                toChangeRoad[road1ID].push_back( make_pair(vehicle.vID, -1) );
                //cout << "   new spawn of vehicle: " << vehicle.getID() << endl; //" from: " << road1ID <<
            }
        }

        Vec3d offset = Vec3d(0,0,0);
        Vec3d dir = vehicle.t->getPose()->dir();
        Vec3d up = vehicle.t->getPose()->up();
        Vec3d left = -dir.cross(up);
        Vec3d right = dir.cross(up);

        //cout << toString(left) << endl;
        float vS = float(vehicle.currentState);
        if (intention==vehicle.STRAIGHT) offset = vehicle.currentOffset + Vec3d(0,0,0);
        if (intention==vehicle.SWITCHLEFT) offset = vehicle.currentOffset*vS + left*vS*d;
        if (intention==vehicle.SWITCHRIGHT) offset = vehicle.currentOffset*vS + right*vS*d;
        //cout << intention << " -- " << toString(vehicle.vID) << " -- " << toString(vehicle.behavior) <<  toString(vehicles[vehicle.vID].behavior) << " -- "<< toString(offset) << endl;

        auto road = roads[gp.edge];
        auto lanewidth = 3; //TODO: should be called from real data
        if (abs(offset.length()) > lanewidth/2 && vS>0.5) {
            roads[vehicle.roadFrom].vehicleIDs.erase(vehicle.vID);
            roads[vehicle.roadTo].vehicleIDs[vehicle.vID] = vehicle.vID;
            vehicle.currentState = -1;
        }
        if (abs(offset.length()) < 0.01 && vS<-0.5) {
            vehicle.roadFrom = -1;
            vehicle.roadTo = -1;
            vehicle.currentState = 0;
        }

        auto p = roadNetwork->getPosition(vehicle.pos);
        //cout << "propagated vehicle pos " <<toString(p) << endl;
        if (offset.length()>20) offset = Vec3d(0,0,0); ///DEBUGGING
        vehicle.lastMove = p->pos() + offset - vehicle.t->getFrom();
        p->setPos(p->pos()+offset);
        vehicle.t->setPose(p);
        vehicle.lastMoveTS = VRGlobals::CURRENT_FRAME;
        vehicle.currentOffset = offset;
        //cout << "Vehicle " << vehicle.vehicleID << " " << p->pos() << " " << vehicle.pos.edge << " " << vehicle.pos.pos << endl;
    };

    auto inFront = [&](PosePtr p1, PosePtr p2, Vec3d lastMove) -> bool {
        lastMove.normalize();
        Vec3d D = p2->pos() - (p1->pos() + lastMove*3);
        float L = D.length();
        Vec3d Dn = D/L;

        float d = Dn.dot(lastMove);
        Vec3d x = lastMove.cross(Vec3d(0,1,0));
        x.normalize();
        float rL = abs( D.dot(x) );

        return d > 0 && L < 5 && rL < 2; // in front, in range, in corridor
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

        return d > 0 && L < 15 && r > 0 && a < -0.3;
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
                for (auto vv : res) {
                    auto v = (Vehicle*)vv;
                    if (!v) continue;
                    if (!v->t) continue;
                    auto p = v->t->getPose();

                    if (inFront(pose, p, vehicle.lastMove)) {
                        state = 1;
                        vehicle.vehiclesight[vehicle.INFRONT] = true;
                        //cout << toString(vehicle.vID) << " infront" <<endl;
                    }
                    else if (comingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (comingLeft(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMLEFT] = true; /*cout << toString(vehicle.vID) << " left" <<endl;*/ }
                    if (comingRight(pose, p, vehicle.lastMove)) { vehicle.vehiclesight[vehicle.FROMRIGHT] = true; /*cout << toString(vehicle.vID) << " right" <<endl;*/ }
                    if (state > 0) break;
                }

                bool debugBool = false; ///Debugging
                if (debugBool) state = 0; //DANGER: debug mode, state = 0, discard collision check

                for (auto& v : users) {
                    auto p = v.t->getPose();
                    if (inFront(pose, p, vehicle.lastMove)) state = 1;
                    else if (comingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (state > 0) break;
                }

                if (auto g = dynamic_pointer_cast<VRGeometry>(vehicle.mesh)) { // only for debugging!!
                    if (state == 0) g->setColor("white");
                    if (state == 1) g->setColor("blue");
                    if (state == 2) g->setColor("red");
                }

                int vbeh = vehicle.behavior;
                if (vbeh == vehicle.STRAIGHT && state == 0) propagateVehicle(vehicle, d, vbeh);
                else if (VRGlobals::CURRENT_FRAME - vehicle.lastMoveTS > 200 ) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle.vID, -1) ); //killswitch if vehicle get's stuck
                }
                if (vbeh == vehicle.SWITCHLEFT  && vehicle.vehiclesight[vehicle.FROMLEFT] == false && vehicle.vehiclesight[vehicle.INFRONT] == false) propagateVehicle(vehicle, d, vbeh);
                if (vbeh == vehicle.SWITCHRIGHT && vehicle.vehiclesight[vehicle.FROMRIGHT] == false && vehicle.vehiclesight[vehicle.INFRONT] == false) propagateVehicle(vehicle, d, vbeh);
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
                if (v.second == -1) { vehicles[v.first].hide(); vehiclePool.push_front(vehicles[v.first]); numUnits--; }
                else {
                    auto& road2 = roads[v.second];
                    road2.vehicleIDs[v.first]=v.first;
                }
            }
        }
    };

    updateSimulationArea();
    fillOctree();
    propagateVehicles();
    //resolveCollisions();
    //updateDensityVisual();
    resolveRoadChanges();
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
            return vehicles[v.vID];
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

            for (auto l : v.turnsignalsBL) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.turnsignalsBR) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.turnsignalsFL) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.turnsignalsFR) l->setMaterial(carLightOrangeBlink);
            for (auto l : v.headlights) l->setMaterial(carLightWhiteOn);
            for (auto l : v.backlights) l->setMaterial(carLightRedOn);
            if (v.getID()==-1) {
                static size_t nID = -1; nID++;
                v.setID(nID);
                vehicles[nID] =v;
                cout<<"VRTrafficSimulation::addVehicle: Added vehicle to map vID:" << v.getID()<<endl;
            }
            return vehicles[v.vID];
        }
    };

    //if () cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    //auto& road = roads[roadID];
    Vehicle v = getVehicle();

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

void VRTrafficSimulation::changeLane(int ID, int direction) {
    auto& v = vehicles[ID];
    auto& gp = v.pos;
    v.roadFrom = gp.edge;
    auto& edge = roadNetwork->getGraph()->getEdge(gp.edge);
    //TODO: should get real left and right detection
    if (edge.relations.size()>0){
        if (direction = 1) v.roadTo = edge.relations[0];
        if (direction = 2 && edge.relations.size()>1) v.roadTo = edge.relations[1];
        else v.roadTo = edge.relations[0];
        v.currentState = 1;
        v.behavior = direction;
    }
    else cout << "VRTrafficSimulation::changeLane: possible lane doesn't exist" << endl;
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
    cout << "Vehicle " << toString(vID) << " set to" << vehicles[vID].behavior << endl;
}

void VRTrafficSimulation::runDiagnostics(){
    string returnInfo = "";
    string nl = "\n ";
    string roadInfo = "ALL ROADS: ";
    string edgeInfo = "ALL EDGES: ";
    string nodeInfo = "ALL NODES: ";
    string vehiInfo = "ALL VEHICLES: ";

    auto fit = [&](int input) {
        string res = "";
        int l1 = toString(input).length();
        for (int i=l1 ; i<4 ; i++) res+=" ";
        return res+toString(input);
    };

    ///get all roads, edges
    int n1 = 0;
    auto graph = roadNetwork->getGraph();
    for (auto eV : graph->getEdges()) {
        auto e = eV.second;
        roadInfo += toString(e.ID) + " ";
        edgeInfo += toString(e.from) +"-"+ toString(e.to) + " ";
        n1++;
    }

    ///get all nodes
    int n2 = 0;
    for (auto n : graph->getNodes()) {
        nodeInfo += toString(n.second.ID) + " ";
        n2++;
    }

    ///get all vehicles
    int n3=0;
    for (auto v : vehicles) {
        vehiInfo += toString(v.second.vID) + " ";
        n3++;
    }

    returnInfo += lastseedRoadsString;
    returnInfo += nl + fit(n1) + "--" + roadInfo;
    returnInfo += nl + fit(n1) + "--" + edgeInfo;
    returnInfo += nl + fit(n2) + "--" + nodeInfo;
    returnInfo += nl + fit(n3) + "--" + vehiInfo;


    CPRINT(returnInfo);
}

bool VRTrafficSimulation::isSeedRoad(int roadID){
    for (auto e : seedRoads){if (roadID==e) return true;}
    return false;
}

void VRTrafficSimulation::setSeedRoad(int debugOverRideSeedRoad){
    this->debugOverRideSeedRoad = debugOverRideSeedRoad;
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



