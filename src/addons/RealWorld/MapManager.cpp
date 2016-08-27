#include "MapManager.h"
#include "World.h"
#include "MapGrid.h"
#include "MapCoordinator.h"
#include <boost/format.hpp>
#include "Modules/BaseModule.h"

#include "OSM/OSMMap.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/objects/object/VRObject.h"

using namespace OSG;

MapManager::MapManager(Vec2f position, MapCoordinator* mapCoordinator, World* world, VRObjectPtr root) {
    this->position = position;
    this->mapCoordinator = mapCoordinator;
    this->world = world;
    this->root = root;

    grid = new MapGrid(3, mapCoordinator->getGridSize() );
}

void MapManager::addModule(BaseModule* mod) {
    modules.push_back(mod);
    root->addChild(mod->getRoot());
}

void MapManager::updatePosition(Vec2f pos) {
    position = pos;
    Vec2f bboxPosition = mapCoordinator->getRealBboxPosition(pos);
    grid->set(bboxPosition);

    // unload/load boxes as needed
    vector<MapGrid::Box> toUnload;
    for (auto b : loadedBoxes) {
        if (!grid->has(b.second)) toUnload.push_back(b.second);
    }

    vector<MapGrid::Box> toLoad;
    for (auto b : grid->getBoxes()) {
        if (!loadedBoxes.count( b.str )) toLoad.push_back(b);
    }

    for (auto b : toUnload) {
        for(auto mod : modules) mod->unloadBbox(b);
        loadedBoxes.erase(b.str);
    }

    for(auto b : toLoad) {
        for(auto mod : modules) mod->loadBbox(b);
        loadedBoxes[b.str] = b;
    }
}

void MapManager::physicalize(bool b) {
    for(auto mod : modules) mod->physicalize(b);
}
