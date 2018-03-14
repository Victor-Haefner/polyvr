#include "VRTrafficSimulation.h"
#include "../roads/VRRoad.h"
#include "../roads/VRRoadNetwork.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
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

using namespace OSG;

template<class T>
void erase(vector<T>& v, const T& t) {
    v.erase(remove(v.begin(), v.end(), t), v.end());
}


VRTrafficSimulation::Vehicle::Vehicle(Graph::position p) : pos(p) {
    t = VRTransform::create("t");
    speed = speed*(1.0+0.2*0.01*(rand()%100));
}

VRTrafficSimulation::Vehicle::~Vehicle() {}

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
}

VRTrafficSimulation::~VRTrafficSimulation() {}

VRTrafficSimulationPtr VRTrafficSimulation::create() { return VRTrafficSimulationPtr( new VRTrafficSimulation() ); }

void VRTrafficSimulation::setRoadNetwork(VRRoadNetworkPtr rds) {
    roadNetwork = rds;
    roads.clear();
    auto graph = roadNetwork->getGraph();
    for (int i = 0; i < graph->getNEdges(); i++) {
        roads[i] = road();
        auto& e = graph->getEdge(i);
        Vec3d p1 = graph->getNode(e.from).p.pos();
        Vec3d p2 = graph->getNode(e.to).p.pos();
        roads[i].length = (p2-p1).length();
    }

    //updateDensityVisual(true);
}

template<class T>
T randomChoice(vector<T> vec) {
    if (vec.size() == 0) return 0;
    return vec[ round((float(random())/RAND_MAX) * (vec.size()-1)) ];
}

void VRTrafficSimulation::updateSimulation() {
    if (!roadNetwork) return;
    auto g = roadNetwork->getGraph();
    auto space = Octree::create(2);
    map<int, vector<pair<Vehicle, int>>> toChangeRoad;
    float userRadius = 150; // x meter radius around users

    auto fillOctree = [&]() {
        for (auto& road : roads) { // fill octree
            for (auto& vehicle : road.second.vehicles) {
                auto pos = vehicle.t->getFrom();
                space->add(pos, &vehicle);
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
            Vec3d p = user.t->getPoseTo(ptr())->pos();
            for (auto eV : graph->getEdges()) {
                for (auto e : eV) {
                    if (graph->getPrevEdges(e).size() == 0) { // roads that start out of "nowhere"
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
            }
        }

        for (auto roadID : makeDiff(nearRoads, newNearRoads)) {
            auto& road = roads[roadID];
            for (auto v : road.vehicles) v.destroy();
            road.vehicles.clear();
            road.macro = true;
        }

        seedRoads = newSeedRoads;
        nearRoads = newNearRoads;

        for (auto roadID : nearRoads) {
            auto& road = roads[roadID];
            road.macro = false;
        }

        for (auto roadID : seedRoads) {
            auto& road = roads[roadID];
            addVehicles(roadID, road.density, 1); // TODO: pass a vehicle type!!
        }
    };

    auto propagateVehicle = [&](Vehicle& vehicle, float d) {
        auto& gp = vehicle.pos;
        gp.pos += d;

        if (gp.pos > 1) {
            gp.pos -= 1;
            int road1ID = gp.edge;
            auto& edge = g->getEdge(gp.edge);
            auto edges = g->getNextEdges(edge);
            if (edges.size() > 0) {
                gp.edge = randomChoice(edges).ID;
                auto& road = roads[gp.edge];
                if (road.macro) toChangeRoad[road1ID].push_back( make_pair(vehicle, -1) );
                else toChangeRoad[road1ID].push_back( make_pair(vehicle, gp.edge) );
            } else toChangeRoad[road1ID].push_back( make_pair(vehicle, -1) );
        }

        auto p = roadNetwork->getPosition(vehicle.pos);
        vehicle.lastMove = p->pos() - vehicle.t->getFrom();
        vehicle.t->setPose(p);
        vehicle.lastMoveTS = VRGlobals::CURRENT_FRAME;
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

    auto commingRight = [&](PosePtr p1, PosePtr p2, Vec3d lastMove) -> bool {
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

    auto propagateVehicles = [&]() {
        int N = 0;
        for (auto& road : roads) {
            for (auto& vehicle : road.second.vehicles) {
                if (!vehicle.t) continue;
                float d = vehicle.speed/road.second.length;

                // check if road ahead is free
                auto pose = vehicle.t->getPose();
                auto res = space->radiusSearch(pose->pos(), 5);
                int state = 0;
                for (auto vv : res) {
                    auto v = (Vehicle*)vv;
                    if (!v) continue;
                    if (!v->t) continue;
                    auto p = v->t->getPose();

                    if (inFront(pose, p, vehicle.lastMove)) state = 1;
                    else if (commingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (state > 0) break;
                }

                for (auto& v : users) {
                    auto p = v.t->getPose();
                    if (inFront(pose, p, vehicle.lastMove)) state = 1;
                    else if (commingRight(pose, p, vehicle.lastMove)) state = 2;
                    if (state > 0) break;
                }

                /*if (auto g = dynamic_pointer_cast<VRGeometry>(vehicle.mesh)) { // only for debugging!!
                    if (state == 0) g->setColor("white");
                    if (state == 1) g->setColor("blue");
                    if (state == 2) g->setColor("red");
                }*/

                if (state == 0) propagateVehicle(vehicle, d);
                else if (VRGlobals::CURRENT_FRAME - vehicle.lastMoveTS > 200 ) {
                    toChangeRoad[road.first].push_back( make_pair(vehicle, -1) );
                }
                N++; // count vehicles!
            }
        }
        //cout << "propagateVehicles, updated " << N << " vehicles" << endl;
    };

    auto resolveRoadChanges = [&]() {
        for (auto r : toChangeRoad) {
            auto& road = roads[r.first];
            for (auto& v : r.second) {
                erase(road.vehicles, v.first);
                if (v.second == -1) v.first.destroy();
                else {
                    auto& road2 = roads[v.second];
                    road2.vehicles.push_back(v.first);
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

void VRTrafficSimulation::addVehicle(int roadID, int type) {
    //if () cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    auto& road = roads[roadID];
    auto v = Vehicle( Graph::position(roadID, 0.0) );
    v.mesh = models[type]->duplicate();

    //if (VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(v.mesh) ) g->makeUnique(); // only for debugging!!
    //v.t->setPickable(true);

    v.t->addChild(v.mesh);
    addChild(v.t);
    road.vehicles.push_back( v );
}

void VRTrafficSimulation::addVehicles(int roadID, float density, int type) {
    auto road = roads[roadID];
    auto g = roadNetwork->getGraph();
    auto e = g->getEdge(roadID);
    int n1 = e.from;
    int n2 = e.to;
    float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
    int N0 = road.vehicles.size();
    int N = L*density/5.0; // density of 1 means one car per 5 meter!
    //cout << "addVehicles N0 " << N0 << " L " << L << " d " << density << " N " << N << " to " << roadID << endl;
    for (int i=N0; i<N; i++) addVehicle(roadID, type);
}

void VRTrafficSimulation::setTrafficDensity(float density, int type) {
    //for (auto road : roads) addVehicles(road.first, density, type);
    for (auto& road : roads) road.second.density = density;
}

int VRTrafficSimulation::addVehicleModel(VRObjectPtr mesh) {
    models.push_back( mesh->duplicate() );
    return models.size()-1;
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
        for (auto n : g->getNodes()) geo.pushVert( n.p.pos() + Vec3d(0,h,0) );
        for (auto eV : g->getEdges()) for (auto e : eV) geo.pushLine(e.from, e.to);
        geo.apply(flowGeo);
    }

    if (flowGeo->size() > 0) { // TODO
        //for (auto road)
    }
}



