#include "RealWorld.h"

#include <math.h>

#include <stdio.h>
#include <curl/curl.h>
#include <iostream>
#include <fstream>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSceneLoader.h"
#include "OSM/OSMMapDB.h"
#include "World.h"
#include "MapCoordinator.h"
#include "MapManager.h"
//#include "TestMover.h"
#include "Modules/ModuleBuildings.h"
#include "Modules/ModuleStreets.h"
#include "Modules/ModuleFloor.h"
#include "Modules/ModuleTree.h"
#include "Modules/ModuleTerrain.h"
#include "Modules/ModuleWalls.h"
#include "Elevation.h"
#include "Config.h"
#include "traffic/ModuleTraffic.h"

#define PI 3.14159265

using namespace OSG;

map<string, string> RealWorld::options = map<string, string>();
RealWorld* RealWorld::singelton = 0;

RealWorld::RealWorld(string name) {
    setName(name);
    singelton = this;
}

RealWorld::~RealWorld() {
    if (world) delete world;
    if (mapDB) delete mapDB;
    if (mapCoordinator) delete mapCoordinator;
    if (mapManager) delete mapManager;
}

std::shared_ptr<RealWorld> RealWorld::create(string name) { return std::shared_ptr<RealWorld>(new RealWorld(name)); }

void RealWorld::init(Vec2i size) {
    mapCoordinator = new MapCoordinator(Vec2f(), 0.002f);
    world = new World();
    mapDB = new OSMMapDB();
    mapManager = new MapManager(Vec2f(), mapCoordinator, world, ptr());
    //mapCoordinator->altitude->showHeightArray(49.000070f, 8.401015f);
}

RealWorld* RealWorld::get() { return singelton; }
MapCoordinator* RealWorld::getCoordinator() { return mapCoordinator; }
MapManager* RealWorld::getManager() { return mapManager; }
World* RealWorld::getWorld() { return world; }
OSMMapDB* RealWorld::getDB() { return mapDB; }
TrafficSimulation* RealWorld::getTrafficSimulation() { return trafficSimulation; }

void RealWorld::update(Vec3f pos) { if (mapManager) mapManager->updatePosition( Vec2f(pos[0], pos[2]) ); }
void RealWorld::configure(string var, string val) { options[var] = val; }
string RealWorld::getOption(string var) { return options[var]; }

void RealWorld::enableModule(string mod, bool b, bool t, bool p) {
    if (!mapManager) return;
    if (b) {
        if (mod == "Ground") mapManager->addModule(new ModuleFloor(t,p));
        if (mod == "Streets") mapManager->addModule(new ModuleStreets(t,p));
        if (mod == "Buildings") mapManager->addModule(new ModuleBuildings(t,p));
        if (mod == "Walls") mapManager->addModule(new ModuleWalls(t,p));
        if (mod == "Terrain") mapManager->addModule(new ModuleTerrain(t,p));
        if (mod == "Trees") mapManager->addModule(new ModuleTree(t,p));
        if (mod == "Traffic") {
            auto tsim = new ModuleTraffic();
            trafficSimulation = tsim->getTrafficSimulation();
            mapManager->addModule(tsim);
        }
    } else {
        // TODO
    }
}



