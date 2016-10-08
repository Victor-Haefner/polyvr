#include "MapManager.h"
#include "MapGrid.h"
#include "MapCoordinator.h"
#include "Modules/BaseModule.h"

#include "core/objects/object/VRObject.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRThreadManager.h"
#include "core/utils/VRFunction.h"

#include <boost/bind.hpp>

using namespace OSG;

MapManager::MapManager(Vec2f position, MapCoordinator* mapCoordinator, World* world, VRObjectPtr root) {
    this->position = position;
    this->mapCoordinator = mapCoordinator;
    this->world = world;
    this->root = root;

    grid = new MapGrid(3, mapCoordinator->getGridSize() );

    worker = VRFunction< VRThreadWeakPtr >::create( "mapmanager work", boost::bind(&MapManager::work, this, _1) );
    VRScene::getCurrent()->initThread(worker, "mapmanager worker", true, 1);
}

void MapManager::work(VRThreadWeakPtr tw) {
    if (jobs.size() == 0) { sleep(1); return; }

    MapManager::job j = jobs.front(); jobs.pop_front();
    VRThreadPtr t = tw.lock();
    t->syncFromMain();
    j.mod->loadBbox(j.b);
    t->syncToMain();
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

    //cout << "MapManager::updatePosition " << pos << " load " << toLoad.size() << " unload " << toUnload.size() << endl;

    for(auto b : toLoad) {
        loadedBoxes[b.str] = b;
        for(auto mod : modules) {
            if (!mod->useThreads) mod->loadBbox(b);
            else jobs.push_back(job(b, mod));
        }
    }

    /*for (auto b : toUnload) { // segfaulting when threaded
        loadedBoxes.erase(b.str);
        for(auto mod : modules) mod->unloadBbox(b);
    }*/
}
