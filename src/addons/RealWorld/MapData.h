#ifndef MAPDATA_H
#define	MAPDATA_H

#include "Modules/StreetJoint.h"
#include "Modules/StreetSegment.h"
#include "Modules/Building.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class MapData {
    public:
        Vec2f boundsMin;
        Vec2f boundsMax;
        vector<StreetJoint*> streetJoints;
        vector<StreetSegment*> streetSegments;
        vector<Building*> buildings;

        MapData* diff(MapData* mapData);
};

OSG_END_NAMESPACE;

#endif	/* MAPDATA_H */

