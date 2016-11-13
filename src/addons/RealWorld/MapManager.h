#ifndef MAPMANAGER_H
#define	MAPMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <OpenSG/OSGVector.h>
#include "MapGrid.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class World;
class MapCoordinator;
class BaseModule;
class AreaBoundingBox;
class MapData;

class MapManager {
    private:
        Vec2f position;
        MapGrid* grid = 0;
        MapCoordinator* mapCoordinator = 0;
        World* world = 0;
        vector<BaseModule*> modules;
        VRObjectPtr root;
        VRThreadCbPtr worker;

        map<string, MapGrid::Box> loadedBoxes;

        struct job {
            MapGrid::Box b;
            BaseModule* mod = 0;
            job(MapGrid::Box& b, BaseModule* m) : b(b), mod(m) {}
        };

        list<job> jobs;

        void work(VRThreadWeakPtr tw);

    public:
        MapManager(Vec2f position, MapCoordinator* mapCoordinator, World* world, VRObjectPtr root);

        void addModule(BaseModule* mod);
        void updatePosition(Vec2f worldPosition);
};

OSG_END_NAMESPACE;

#endif	/* MAPMANAGER_H */

