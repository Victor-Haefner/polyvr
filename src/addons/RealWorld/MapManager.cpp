#include "MapManager.h"
#include "World.h"
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
}

void MapManager::addModule(BaseModule* mod) {
    modules.push_back(mod);
    root->addChild(mod->getRoot());
}

void MapManager::updatePosition(Vec2f worldPosition) {
    position = worldPosition;
    Vec2f bboxPosition = mapCoordinator->getRealBboxPosition(worldPosition);

    float gs = mapCoordinator->getGridSize();
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


MapData* MapManager::loadMap(string filename) {

    // check if file exists
    ifstream ifile(filename.c_str());
    if (ifile) ifile.close();
    else return NULL;

    MapData* mapData = new MapData();
    OSMMap* osmMap = OSMMap::loadMap(filename);
    map<string, StreetJoint*> streetJointMap;

    // Load Bounds
    mapData->boundsMin = mapCoordinator->realToWorld(Vec2f(osmMap->boundsMinLat, osmMap->boundsMinLon));
    mapData->boundsMax = mapCoordinator->realToWorld(Vec2f(osmMap->boundsMaxLat, osmMap->boundsMaxLon));

    // Load StreetJoints
    for(OSMNode* node : osmMap->osmNodes) {
        Vec2f pos = mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
        StreetJoint* joint = new StreetJoint(pos, node->id);
        mapData->streetJoints.push_back(joint);
        joint->info = filename;

        streetJointMap[node->id] = joint;
    }

    // Load StreetSegments && Buildings
    for(OSMWay* way : osmMap->osmWays) {
        if (way->tags["building"] == "yes") {
            // load building
            Building* b = new Building(way->id);
            for(string nodeId : way->nodeRefs) {
                OSMNode* node = osmMap->osmNodeMap[nodeId];
                Vec2f pos = mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
                b->positions.push_back(pos);
            }
            mapData->buildings.push_back(b);
        } else if (
                way->tags["highway"] == "motorway" ||
                way->tags["highway"] == "trunk" ||
                way->tags["highway"] == "primary" ||
                way->tags["highway"] == "secondary" ||
                way->tags["highway"] == "tertiary" ||
                way->tags["highway"] == "living_street" ||
                way->tags["highway"] == "residential" ||
                way->tags["highway"] == "unclassified" ||
                way->tags["highway"] == "road") {
            // load street segment
            for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
                string nodeId1 = way->nodeRefs[i];
                string nodeId2 = way->nodeRefs[i+1];
                string segId = way->id + "-" + toString(i);

                StreetSegment* seg = new StreetSegment(nodeId1, nodeId2, 2.1f, segId);
                mapData->streetSegments.push_back(seg);

                if (way->tags.count("lanes")) {
                    seg->lanes = toFloat(way->tags["lanes"].c_str());
                    seg->width = seg->width * seg->lanes;
                } else if (way->tags["highway"] == "secondary") {
                    seg->lanes = 2;
                    seg->width = seg->width * seg->lanes;
                }

                if (way->tags.count("name")) {
                    seg->name = way->tags["name"];
                }

                streetJointMap[nodeId1]->segmentIds.push_back(segId);
                streetJointMap[nodeId2]->segmentIds.push_back(segId);
            }
        }
    }

    return mapData;
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
