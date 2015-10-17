#ifndef MAPLOADER_H
#define	MAPLOADER_H

#include "MapCoordinator.h"
#include "MapData.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class MapLoader {
    public:
        MapCoordinator* mapCoordinator;

        MapLoader(MapCoordinator* mapCoordinator);

        MapData* loadMap(string filename);
};

OSG_END_NAMESPACE;

#endif	/* MAPLOADER_H */

