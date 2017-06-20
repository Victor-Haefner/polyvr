#ifndef VRGRASSPATCH_H_INCLUDED
#define VRGRASSPATCH_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"
#include "addons/RealWorld/VRRealWorldFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeoData;

class VRGrassPatch : public VRTransform {
    private:
        PolygonPtr area;
        vector<PolygonPtr> chunks;
        VRLodPtr lod;
        map<int, VRGeometryPtr> lods;
        float bladeHeight = 0.3;
        static VRTextureRendererPtr texRenderer;
        static VRPlantMaterialPtr matGrassSide;
        static VRPlantMaterialPtr matGrass;

        void setupGrassMaterial();
        void setupGrassStage();

        void initLOD();
        void addGrassBlade(VRGeoData& data, Vec3f pos, float a, float dh, int lvl, Vec3f c);
        void createPatch(VRGeoData& data, PolygonPtr area, int lvl = 0, int density = 100);
        void createSpriteLOD(VRGeoData& data, PolygonPtr area, int lvl);

    public:
        VRGrassPatch();
        ~VRGrassPatch();
        static VRGrassPatchPtr create();

        void setArea(PolygonPtr p);

        void createLod(VRGeoData& geo, int lvl, Vec3f offset, int ID);

        static VRMaterialPtr getGrassMaterial();
        static VRMaterialPtr getGrassSideMaterial();
};

OSG_END_NAMESPACE;

#endif // VRGRASSPATCH_H_INCLUDED
