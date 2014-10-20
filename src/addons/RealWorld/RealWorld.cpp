#include "RealWorld.h"

#include <math.h>

#include <stdio.h>
#include <curl/curl.h>
#include <iostream>
#include <fstream>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSceneLoader.h"
#include "OSM/OSMMapDB.h"
#include "MapCoordinator.h"
#include "MapLoader.h"
#include "MapManager.h"
//#include "TestMover.h"
#include "TextureManager.h"
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

using namespace realworld;


RealWorld::RealWorld(VRObject* root) {
    physicalized = false;

    cout << "\nINIT REALWORLD\n";

    Timer t;
    t.start("Program loading time");

    mapCoordinator = new MapCoordinator(OSG::Vec2f(49.005f, 8.395f), 0.002f); //Kreuzung Kriegsstr. und Karlstr.
    //mapCoordinator = new MapCoordinator(OSG::Vec2f(48.998969, 8.400171), 0.002f); // Tiergarten
    //mapCoordinator = new MapCoordinator(OSG::Vec2f(49.013606f, 8.418295f), 0.002f); // Fasanengarten, funktioniert nicht?
    //mapCoordinator = new MapCoordinator(Config::getStartPosition(), Config::gridSize);

    //Vec2f camPos = mapCoordinator->realToWorld(Vec2f(49, 8.4f));
    TextureManager* texManager = new TextureManager(); // load textures (3 images)
    mapGeometryGenerator = new MapGeometryGenerator(texManager);
    world = new World();
    mapDB = new OSMMapDB();
    mapLoader = new MapLoader(mapCoordinator);
    mapManager = new MapManager(Vec2f(0,0), mapLoader, mapGeometryGenerator, mapCoordinator, world, root);

    mapManager->addModule(new ModuleFloor(mapCoordinator, texManager)); // ca. 200ms - 2000ms Abhängig von der Tesselierung
//
    mapManager->addModule(new ModuleBuildings(mapDB, mapCoordinator, texManager)); //ca 500ms - 3000ms Abhängig von Anzahl der Gebäude
    //mapManager->addModule(new ModuleStreets(mapDB, mapCoordinator, texManager)); //ca 9.000ms - 20.000ms Abhängig von Anzahl der Straßen
    mapManager->addModule(new ModuleWalls(mapDB, mapCoordinator, texManager)); //ca. 200ms
    mapManager->addModule(new ModuleTerrain(mapDB, mapCoordinator, texManager)); // 500ms - 3000ms abhängig von Anzahl der Terrains und maximaler Dreieckseitenlänge
    mapManager->addModule(new ModuleTree(mapDB, mapCoordinator, texManager));
    ModuleTraffic *traffic = new ModuleTraffic(mapDB, mapCoordinator, texManager);
    trafficSimulation = traffic->getTrafficSimulation();
    mapManager->addModule(traffic);

    // Make Test Movement
    //new TestMover(scene, scene->getActiveCamera());

    update(Vec3f(0,0,0));
    t.printTime("Program loading time");

    //mapCoordinator->altitude->showHeightArray(49.000070f, 8.401015f);
}

RealWorld::~RealWorld() {
    cout << "\nDESTRUCT REALWORLD\n";

    delete texManager;
    delete mapLoader;
    delete mapDB;
    delete world;
    delete mapGeometryGenerator;
    delete mapCoordinator;
    delete mapManager;
}

void RealWorld::update(OSG::Vec3f pos) { mapManager->updatePosition( OSG::Vec2f(pos[0], pos[2]) ); }


void RealWorld::physicalize(bool b) {
    if (physicalized == b) return;
    physicalized = b;

    mapManager->physicalize(b);
}

TrafficSimulation *RealWorld::getTrafficSimulation() { return trafficSimulation; }
