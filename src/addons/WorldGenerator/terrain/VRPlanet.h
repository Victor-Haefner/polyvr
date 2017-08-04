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

        double radius = 6371000; // earth radius
        map<int, map<int, VRWorldGeneratorPtr> > sectors;
        VRLodPtr lod;
        VRObjectPtr anchor;
        VRMaterialPtr sphereMat;

        VRAnalyticGeometryPtr metaGeo;

        double toRad(double deg);
        double toDeg(double rad);
        void rebuild();

    public:
        VRPlanet(string name);
        ~VRPlanet();

        VRPlanetPtr ptr();
        static VRPlanetPtr create(string name = "planet");

        void setParameters( double radius );
        VRWorldGeneratorPtr addSector( int north, int east );
        VRWorldGeneratorPtr getSector( double north, double east );
        int addPin( string label, double north, double east );
        void remPin( int pin );

        Vec3d fromLatLongEast(double north, double east);
        Vec3d fromLatLongNorth(double north, double east);
        Vec3d fromLatLongNormal(double north, double east);
        Vec3d fromLatLongPosition(double north, double east);
        Vec2d fromLatLongSize(double north1, double east1, double north2, double east2);

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRPLANET_H_INCLUDED
