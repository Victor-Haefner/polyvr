#include "VRTrafficSimulation.h"
#include "../roads/VRRoad.h"
#include "../roads/VRRoadNetwork.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/polygon.h"
#include "core/math/graph.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRObjectManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"

#include <boost/bind.hpp>

using namespace OSG;


VRTrafficSimulation::vehicle::vehicle(Graph::position p) : pos(p) {
    t = VRTransform::create("t");
}

VRTrafficSimulation::vehicle::~vehicle() {}


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
    auto g = roadNetwork->getGraph();
    for (int i = 0; i < g->getNEdges(); i++) roads[i] = road();
}

void VRTrafficSimulation::updateSimulation() {
    cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    for (int i=0; i<roads.size(); i++) {
        auto road = roads[i];
        if (i == 0) cout << " VRTrafficSimulation::updateSimulation road0 " << road.vehicles.size() << endl;
        for (auto& vehicle : road.vehicles) {
            vehicle.pos.pos += 0.1;
            if (vehicle.pos.pos > 1) vehicle.pos.pos = 0;
            auto p = roadNetwork->getPosition(vehicle.pos);
            if (i == 0) cout << "  VRTrafficSimulation::updateSimulation road0 " << p->pos() << endl;
            vehicle.t->setPose(p);
        }
    }
}

void VRTrafficSimulation::addVehicle(int roadID, int type) {
    auto road = roads[roadID];
    auto v = vehicle( Graph::position(roadID, 0.0) );
    v.mesh = models[0]->duplicate();
    v.t->addChild(v.mesh);
    addChild(v.t);
    road.vehicles.push_back( v );
}

void VRTrafficSimulation::addVehicles(int roadID, float density, int type) {
    auto g = roadNetwork->getGraph();
    auto e = g->getEdge(roadID);
    int n1 = e.from;
    int n2 = e.to;
    float L = (g->getNode(n2).p.pos() - g->getNode(n1).p.pos()).length();
    int N = L*density/5.0; // density of 1 means one car per 5 meter!
    for (int i=0; i<N; i++) addVehicle(roadID, type);
}

void VRTrafficSimulation::setTraffic(float density, int type) {
    for (auto road : roads) addVehicles(road.first, density, type);
}

void VRTrafficSimulation::addVehicleModel(VRObjectPtr mesh) {
    models.push_back( mesh->duplicate() );
}




