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
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRObjectManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"

#include <boost/bind.hpp>

using namespace OSG;


VRTrafficSimulation::Vehicle::Vehicle(Graph::position p) : pos(p) {
    t = VRTransform::create("t");
}

VRTrafficSimulation::Vehicle::~Vehicle() {}


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
    updateDensityVisual(true);
}

void VRTrafficSimulation::updateSimulation() {
    auto g = roadNetwork->getGraph();

    auto propagateVehicle = [&](Vehicle& vehicle) {
        auto& gp = vehicle.pos;
        gp.pos += 0.005;
        if (gp.pos > 1) {
            gp.pos -= 1;
            auto& edge = g->getEdge(gp.edge);
            auto edges = g->getNextEdges(edge);
            if (edges.size() > 0) gp.edge = edges[0].ID;
            else {
                for (auto eV : g->getEdges()) for (auto e : eV) if (g->getPrevEdges(e).size() == 0) gp.edge = e.ID;
            }
        }
    };


    for (int i=0; i<roads.size(); i++) {
        auto& road = roads[i];
        for (auto& vehicle : road.vehicles) {
            propagateVehicle(vehicle);
            auto p = roadNetwork->getPosition(vehicle.pos);
            vehicle.t->setPose(p);
        }
    }

    updateDensityVisual();
}

void VRTrafficSimulation::addVehicle(int roadID, int type) {
    //if () cout << "VRTrafficSimulation::updateSimulation " << roads.size() << endl;
    auto& road = roads[roadID];
    auto v = Vehicle( Graph::position(roadID, 0.0) );
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

void VRTrafficSimulation::setTrafficDensity(float density, int type) {
    for (auto road : roads) addVehicles(road.first, density, type);
}

void VRTrafficSimulation::addVehicleModel(VRObjectPtr mesh) {
    models.push_back( mesh->duplicate() );
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



