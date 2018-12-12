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

        double sectorSize = 0.1; // in spherical coordinates

        double radius = 6371000; // earth radius
        map<Vec2i, VRWorldGeneratorPtr> sectors;
        Vec2d originCoords = Vec2d(-1,-1);
        VRTransformPtr origin;
        VRLodPtr lod;
        VRObjectPtr anchor;
        VRMaterialPtr sphereMat;

        VRAnalyticGeometryPtr metaGeo;
        void rebuild();
        Vec2i toSID(double north, double east);

    public:
        VRPlanet(string name);
        ~VRPlanet();

        VRPlanetPtr ptr();
        static VRPlanetPtr create(string name = "planet");

        double toRad(double deg);
        double toDeg(double rad);

        void setParameters( double radius, double sectorSize = 0.1 );
        VRWorldGeneratorPtr addSector( double north, double east );
        VRWorldGeneratorPtr getSector( double north, double east );
        vector<VRWorldGeneratorPtr> getSectors();
        int addPin( string label, double north, double east, double length = 10000 );
        void remPin( int pin );

        Vec3d fromLatLongEast(double north, double east, bool local = false);
        Vec3d fromLatLongNorth(double north, double east, bool local = false);
        Vec3d fromLatLongNormal(double north, double east, bool local = false);
        Vec3d fromLatLongPosition(double north, double east, bool local = false);
        Vec2d fromLatLongSize(double north1, double east1, double north2, double east2);
        PosePtr fromLatLongPose(double north, double east, bool local = false);

        Vec2d fromPosLatLong(Pnt3d p, bool local = false);

        void localize(double north, double east);

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRPLANET_H_INCLUDED
