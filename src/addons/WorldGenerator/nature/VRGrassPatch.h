#ifndef VRGRASSPATCH_H_INCLUDED
#define VRGRASSPATCH_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"
#include "addons/RealWorld/VRRealWorldFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeoData;

class VRGrassPatch : public VRTransform {
    private:
        VRMaterialPtr mat;
        PolygonPtr area;
        map<int, VRGeometryPtr> lods;

        void addGrassBlade(VRGeoData& data);

    public:
        VRGrassPatch();
        ~VRGrassPatch();
        static VRGrassPatchPtr create();

        void setArea(PolygonPtr p);

        void createLod(VRGeoData& geo, int lvl, Vec3f offset, int ID);
};

OSG_END_NAMESPACE;

#endif // VRGRASSPATCH_H_INCLUDED
