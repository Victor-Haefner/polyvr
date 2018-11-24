#ifndef VRBUILDING_H
#define	VRBUILDING_H

#include "../VRWorldGeneratorFwd.h"
#include "../VRWorldModule.h"
#include "core/math/polygon.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

/*struct BuildingStructure {
    vector<Vec2d> vertices;
    vector<int*> faces;
};*/

class VRBuilding : public VRWorldModule {
    private:
        string type = "residential";
        int wallType = 0;
        int windowType = 0;
        int doorType = 0;
        int roofType = 0;
        float height = 0;
        float ground = 0;

        VRPolygon roof;
        vector<pair<float,VRPolygon>> foundations;
        vector<pair<float,VRPolygon>> stories;

    public:
        VRBuilding();
        ~VRBuilding();

        static VRBuildingPtr create();

        void setType(string type);

        void addFoundation(VRPolygon polygon, float H);
        void addFloor(VRPolygon polygon, float H);
        void addRoof(VRPolygon polygon);

        void computeGeometry(VRGeometryPtr walls, VRGeometryPtr roofs, VRDistrictPtr district);
        VRGeometryPtr getCollisionShape();
};

OSG_END_NAMESPACE;


#endif	/* VRBUILDING_H */

