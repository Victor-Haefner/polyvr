#ifndef VRPLANET_H_INCLUDED
#define VRPLANET_H_INCLUDED

#include "core/objects/VRTransform.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPlanet : public VRTransform {
    private:
        static string surfaceVP;
        static string surfaceFP;

        double sectorSize = 0.1; // in spherical coordinates
        int layermode = 0;

        double radius = 6371000; // earth radius
        double rotation = 0.99; // earth day
        double inclination = 0.4; // earth inclination in rad
        map<Vec2i, VRWorldGeneratorPtr> sectors;
        map<string, OSMMapPtr> osmMaps;
        Vec2d originCoords = Vec2d(-1,-1);
        bool localized = false;
        VRTransformPtr origin;
        VRLodPtr lod;
        VRObjectPtr anchor;
        VRMaterialPtr sphereMat;

        vector<VRTransformPtr> moons;

        VRAnalyticGeometryPtr metaGeo;
        void rebuild();
        void setupMetaGeo();
        Vec2i toSID(double north, double east);

    public:
        VRPlanet(string name);
        ~VRPlanet();

        VRPlanetPtr ptr();
        static VRPlanetPtr create(string name = "planet");

        double toRad(double deg);
        double toDeg(double rad);

        void setParameters( double radius, string texture, bool isLit, double sectorSize = 0.1 );
        void setLayermode( string mode );
        VRWorldGeneratorPtr addSector( double north, double east, bool local = false );
        OSMMapPtr addOSMMap( string path );
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
        PosePtr fromLatLongElevationPose(double north, double east, double elevation, bool local = false, bool sectorLocal = false );

        double getRadius();
        double getSectorSize();
        Vec2d getSurfaceUV(double north, double east);
        //double getSurfaceHeight(double north, double east);
        PosePtr getSurfacePose(double north, double east, bool local = false, bool sectorLocal = false );

        Vec2d fromPosLatLong(Pnt3d p, bool local = false);

        void localize(double north, double east);
        void localizeSector(VRWorldGeneratorPtr s);

        VRMaterialPtr getMaterial();
        void setupMaterial(string texture, bool isLit);
        void setLit(bool b);

        void divideTIFF(string pathIn, string pathOut, double minLat, double maxLat, double minLon, double maxLon, double res);
        void divideTIFFEPSG(string pathIn, string pathOut, double minNorthing, double minEasting, double maxNorthing, double maxEasting, double pixelResolution,double chunkResolution, bool debug);

        void setRotation(double rDays);
        void setInclination(double I);
        double getRotation();
        double getInclination();

        void addMoon(VRTransformPtr t);
        vector<VRTransformPtr> getMoons();
};

OSG_END_NAMESPACE;

#endif // VRPLANET_H_INCLUDED
