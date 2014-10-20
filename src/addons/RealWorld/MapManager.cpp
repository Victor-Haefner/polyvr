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
    this->position = worldPosition;
    Vec2f bboxPosition = this->mapCoordinator->getRealBboxPosition(worldPosition);

    float gs = this->mapCoordinator->gridSize;
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
    for(map<string,AreaBoundingBox*>::iterator iter = loadedBboxes.begin(); iter != loadedBboxes.end(); ++iter) {
        string bboxStr = iter->first;
        AreaBoundingBox* bbox = iter->second;
        bool isInBboxes = false;

        BOOST_FOREACH(AreaBoundingBox* bbox2, bboxes) {
            if (bbox2->str == bboxStr) isInBboxes = true;
        }

        if (!isInBboxes) {
            this->unloadBbox(bbox);
        }
    }

    // load new bounding boxes
    BOOST_FOREACH(AreaBoundingBox* bbox, bboxes) {
        this->loadBboxIfNecessary(bbox);
    }
}

void MapManager::unloadBbox(AreaBoundingBox* bbox) {
    cout << "Unloading area: " << bbox->str << "\n" << flush;

    BOOST_FOREACH(BaseModule* mod, this->modules) {
        mod->unloadBbox(bbox);
    }

//            MapData* mapData = this->loadedBboxes[posStr];
//
//            map<string, MapData*>::iterator iter;
//            for (iter = this->loadedBboxes.begin(); iter != this->loadedBboxes.end(); ++iter) {
//                if (iter->first == posStr) continue;
//                MapData* mapData2 = iter->second;
//                mapData = mapData->diff(mapData2);
//            }
//
//            this->world->unloadMapData(mapData);
//            this->mapGeometryGenerator->updateWorldGeometry(this->world);
//            this->world->removeMapData(mapData);

    this->loadedBboxes.erase(bbox->str);
}

void MapManager::loadBboxIfNecessary(AreaBoundingBox* bbox) {
    // stop if bbox is already loaded
    if (this->loadedBboxes.find(bbox->str) != this->loadedBboxes.end())
        return;

    //cout << "Loading area: " << bbox->str << "  MODCOUNT=" << this->modules.size() << "\n" << flush;
    Timer t;
    BOOST_FOREACH(BaseModule* mod, this->modules) {
        t.start("LOAD MOD " + mod->getName() + " FOR " + bbox->str);
        mod->loadBbox(bbox);
        t.printTime("LOAD MOD " + mod->getName() + " FOR " + bbox->str);
    }

    this->loadedBboxes[bbox->str] = bbox;

//            Timer t;
//            t.start("mm-loadBBox");
//
//            t.start("mm-loadMapData");
//            MapData* mapData = this->loadMapData(posStr);
//            t.printTime("mm-loadMapData");
//
//            if (mapData == NULL) {
//                printf("\nFAILED TO LOAD DATA FOR BBOX: %s\n", posStr.c_str());
//                return;
//            }
//            this->loadedBboxes[posStr] = mapData;
//
//            t.start("mm-addMapData");
//            this->world->addMapData(mapData);
//            t.printTime("mm-addMapData");
//
//            t.start("mm-updateWorldGeometry");
//            this->mapGeometryGenerator->updateWorldGeometry(this->world);
//            t.printTime("mm-updateWorldGeometry");
//
//            printf("\nLoaded map-data %s ************\n\n", posStr.c_str());
//            t.printTime("mm-loadBBox");
}

void MapManager::physicalize(bool b) {
    for (uint i=0; i<modules.size(); i++) {
        modules[i]->physicalize(b);
    }
}
