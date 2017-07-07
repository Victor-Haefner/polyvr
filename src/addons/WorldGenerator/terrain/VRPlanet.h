#ifndef VRPLANET_H_INCLUDED
#define VRPLANET_H_INCLUDED

#include "core/objects/VRTransform.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPlanet : public VRTransform {
    private:
        static string surfaceVP;
        static string surfaceFP;

        float radius = 6371; // earth radius
        map<int, map<int, VRTerrainPtr> > sectors;
        VRLodPtr lod;
        VRObjectPtr anchor;
        VRMaterialPtr sphereMat;

        float toRad(float deg);
        float toDeg(float rad);
        void rebuild();

        Vec3f fromLatLongEast(float north, float east);
        Vec3f fromLatLongSouth(float north, float east);
        Vec3f fromLatLongNormal(float north, float east);
        Vec3f fromLatLongPosition(float north, float east);
        Vec2f fromLatLongSize(float north1, float east1, float north2, float east2);

    public:
        VRPlanet(string name);
        ~VRPlanet();
        static VRPlanetPtr create(string name = "planet");

        void setParameters( float radius );
        VRTerrainPtr addSector( int north, int east );

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRPLANET_H_INCLUDED
