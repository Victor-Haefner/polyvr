#ifndef MAPMANAGER_H
#define	MAPMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class World;
class MapGeometryGenerator;
class MapCoordinator;
class BaseModule;
class AreaBoundingBox;
class MapData;

class MapManager {
    public:
        Vec2f position;
        MapGeometryGenerator* mapGeometryGenerator;
        MapCoordinator* mapCoordinator;
        World* world;
        vector<BaseModule*> modules;
        VRObjectPtr root;

        map<string, AreaBoundingBox*> loadedBboxes;

        MapManager(Vec2f position, MapGeometryGenerator* mapGeometryGenerator, MapCoordinator* mapCoordinator, World* world, VRObjectPtr root);

        void addModule(BaseModule* mod);

        void updatePosition(Vec2f worldPosition);

        void physicalize(bool b);

    private:
        void unloadBbox(AreaBoundingBox* bbox);

        void loadBboxIfNecessary(AreaBoundingBox* bbox);

        MapData* loadMap(string filename);
};

OSG_END_NAMESPACE;

#endif	/* MAPMANAGER_H */

