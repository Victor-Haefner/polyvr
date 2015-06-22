#include "MapManager.h"

using namespace realworld;

MapManager::MapManager(Vec2f position, MapLoader* mapLoader, MapGeometryGenerator* mapGeometryGenerator, MapCoordinator* mapCoordinator, World* world, VRObject* root) {
    this->position = position;
    this->mapLoader = mapLoader;
    this->mapGeometryGenerator = mapGeometryGenerator;
    this->mapCoordinator = mapCoordinator;
    this->world = world;
    this->root = root;
}

void MapManager::addModule(BaseModule* mod) {
    modules.push_back(mod);
    root->addChild(mod->getRoot());
}

void MapManager::updatePosition(Vec2f worldPosition) {
    position = worldPosition;
    Vec2f bboxPosition = mapCoordinator->getRealBboxPosition(worldPosition);

    float gs = mapCoordinator->gridSize;
    vector<AreaBoundingBox*> bboxes;
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(-gs, -gs), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(0, -gs), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(+gs, -gs), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(-gs, 0), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(0, 0), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(+gs, 0), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(-gs, +gs), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(0, +gs), gs));
    bboxes.push_back(new AreaBoundingBox(bboxPosition + Vec2f(+gs, +gs), gs));

    // unload all bounding boxes which are not in bboxes
    vector<AreaBoundingBox*> toUnload;
    for(auto itr : loadedBboxes) {
        bool isInBboxes = false;
        for(auto bbox2 : bboxes) if (bbox2->str == itr.first) isInBboxes = true;
        if (!isInBboxes) toUnload.push_back(itr.second);
    }

    for (auto b : toUnload) unloadBbox(b);
    for(auto bbox : bboxes) loadBboxIfNecessary(bbox); // load new bounding boxes
}

void MapManager::unloadBbox(AreaBoundingBox* bbox) {
    cout << "Unloading area: " << bbox->str << "\n" << flush;
    for(auto mod : modules) mod->unloadBbox(bbox);
    loadedBboxes.erase(bbox->str);
}

void MapManager::loadBboxIfNecessary(AreaBoundingBox* bbox) {
    if (loadedBboxes.count(bbox->str)) return; // stop if bbox is already loaded
    for(auto mod : modules) mod->loadBbox(bbox);
    loadedBboxes[bbox->str] = bbox;
}

void MapManager::physicalize(bool b) {
    for(auto mod : modules) mod->physicalize(b);
}
