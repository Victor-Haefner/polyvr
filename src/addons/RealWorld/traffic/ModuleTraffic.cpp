#include "ModuleTraffic.h"
#include "core/scene/VRThreadManager.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;


ModuleTraffic::ModuleTraffic(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
    this->mapDB = mapDB;
    this->simulation = new TrafficSimulation(mapCoordinator);
}

ModuleTraffic::~ModuleTraffic() {
    delete simulation;
}

string ModuleTraffic::getName() { return "ModuleTraffic"; }


TrafficSimulation* ModuleTraffic::getTrafficSimulation() {
    return simulation;
}

void ModuleTraffic::loadBbox(AreaBoundingBox* bbox) {
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    VRFunction<VRThreadWeakPtr>* func = new VRFunction<VRThreadWeakPtr>("trafficAddMap", boost::bind(&TrafficSimulation::addMap, simulation, osmMap));
    VRSceneManager::get()->initThread(func, "trafficAddMap", false);
}

void ModuleTraffic::unloadBbox(AreaBoundingBox* bbox) {
    OSMMap* osmMap = mapDB->getMap(bbox->str);
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
