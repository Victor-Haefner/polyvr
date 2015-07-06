#ifndef MODULESTREETS_H
#define MODULESTREETS_H

#include "BaseModule.h"
#include <OpenSG/OSGVector.h>

using namespace OSG;
using namespace std;

namespace OSG {
    class VRGeometry;
    class VRMaterial;
}

namespace realworld {
    class GeometryData;
    class OSMMapDB;
    class MapCoordinator;
    class TextureManager;
    class StreetJoint;
    class StreetSegment;

    class ModuleStreets: public BaseModule {
        public:
            ModuleStreets(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager);

            virtual string getName();

            virtual void loadBbox(AreaBoundingBox* bbox);
            virtual void unloadBbox(AreaBoundingBox* bbox);
            void physicalize(bool b);

            Vec3f elevate(Vec2f p, float h);

        private:
            OSMMapDB* mapDB;
            map<string, VRGeometry*> meshes;
            map<string, vector<VRGeometry*> > signs;
            VRMaterial* matStreet = 0;

            VRGeometry* makeSignGeometry(StreetSegment* seg);
            void makeStreetSegmentGeometry(StreetSegment* s, GeometryData* geo);
            void makeStreetJointGeometry(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo);
            Vec3f getNormal3D(Vec3f v1, Vec3f v2);

            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo);
            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide);
            void pushTriangle(Vec3f c, Vec3f a1, Vec3f a2, Vec3f normal, GeometryData* geo);
    };
}

#endif // MODULESTREETS_H



