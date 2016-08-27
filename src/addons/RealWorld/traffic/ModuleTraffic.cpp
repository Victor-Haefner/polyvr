#include "ModuleTraffic.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include "../OSM/OSMMapDB.h"
#include "../RealWorld.h"
#include <boost/bind.hpp>

using namespace OSG;


ModuleTraffic::ModuleTraffic() : BaseModule("ModuleTraffic", false, false) {
    auto mc = RealWorld::get()->getCoordinator();
    this->simulation = new TrafficSimulation(mc);
}

ModuleTraffic::~ModuleTraffic() {
    delete simulation;
}

TrafficSimulation* ModuleTraffic::getTrafficSimulation() {
    return simulation;
}

void ModuleTraffic::loadBbox(MapGrid::Box bbox) {
    auto mapDB = RealWorld::get()->getDB();
    OSMMap* osmMap = mapDB->getMap(bbox.str);
    if (!osmMap) return;

    threadFkt = VRFunction<VRThreadWeakPtr>::create("trafficAddMap", boost::bind(&TrafficSimulation::addMap, simulation, osmMap));
    VRSceneManager::get()->initThread(threadFkt, "trafficAddMap", false);
}

void ModuleTraffic::unloadBbox(MapGrid::Box bbox) {
    auto mapDB = RealWorld::get()->getDB();
    OSMMap* osmMap = mapDB->getMap(bbox.str);
    if (!osmMap) return;

    // Not inside a thread since osmMap might no longer be valid after this method returns
    simulation->removeMap(osmMap);
}

void ModuleTraffic::physicalize(bool b) {
    return;
}

TrafficSimulation* ModuleTraffic::getSimulation() {
    return simulation;
}
