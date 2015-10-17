#ifndef MODULESTREETS_H
#define MODULESTREETS_H

#include "BaseModule.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGVector.h>

class TextureManager;
class OSMMapDB;

OSG_BEGIN_NAMESPACE;
using namespace std;

class GeometryData;
class MapCoordinator;
class StreetJoint;
class StreetSegment;

class ModuleStreets: public BaseModule {
    public:
        ModuleStreets(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, World* world);

        virtual string getName();

        virtual void loadBbox(AreaBoundingBox* bbox);
        virtual void unloadBbox(AreaBoundingBox* bbox);
        void physicalize(bool b);

        Vec3f elevate(Vec2f p, float h);

    private:
        OSMMapDB* mapDB;
        map<string, VRGeometryPtr> meshes;
        map<string, vector<VRGeometryPtr> > signs;
        VRMaterialPtr matStreet = 0;

        VRGeometryPtr makeSignGeometry(StreetSegment* seg);
        void makeStreetSegmentGeometry(StreetSegment* s, GeometryData* geo);
        void makeStreetJointGeometry(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo);
        Vec3f getNormal3D(Vec3f v1, Vec3f v2);

        void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo);
        void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide);
        void pushTriangle(Vec3f c, Vec3f a1, Vec3f a2, Vec3f normal, GeometryData* geo);
};

OSG_END_NAMESPACE;

#endif // MODULESTREETS_H



