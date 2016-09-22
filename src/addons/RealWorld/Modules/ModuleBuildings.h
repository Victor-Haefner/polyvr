#ifndef MODULEBUILDINGS_H
#define MODULEBUILDINGS_H

#include "BaseModule.h"
#include <map>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct BuildingData;
class Building;
class VRGeoData;
class AreaBoundingBox;

class ModuleBuildings: public BaseModule {
    private:
        map<string, VRGeometryPtr> b_geos;
        map<string, VRGeometryPtr> r_geos;
        VRMaterialPtr b_mat = 0;

        void createBuildingPart(BuildingData* bData, string part, string filePath);
        void addBuildingWallLevel(VRGeoData* b_geo_d, Vec2f pos1, Vec2f pos2, int level, int bNum, float elevation);
        void addBuildingRoof(VRGeoData* r_geo_d, Building* building, float height, float elevation);
        void makeBuildingGeometry(VRGeoData* b_geo_d, VRGeoData* r_geo_d, Building* b); /** create one Building **/

    public:
        ModuleBuildings(bool t, bool p);

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);
};

OSG_END_NAMESPACE;

#endif // MODULEBUILDINGS_H

