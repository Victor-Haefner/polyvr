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

        private:
            OSMMapDB* mapDB;
            map<string, VRGeometry*> meshes;
            map<string, VRGeometry*>::iterator mesh_itr;
            VRMaterial* matStreet;
            VRMaterial* matSign;
            map<string, StreetJoint*> streetJointMap;
            map<string, StreetSegment*> streetSegmentMap;


            void makeSignGeometry(StreetSegment* seg, GeometryData* geo);
            void makeStreetSegmentGeometry(StreetSegment* s, GeometryData* geo);
            void makeStreetJointGeometry(StreetJoint* sj, GeometryData* geo);
            Vec3f getNormal3D(Vec3f v1, Vec3f v2);

            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo);
            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide);
            void pushTriangle(Vec3f c, Vec3f a1, Vec3f a2, Vec3f normal, GeometryData* geo);
    };
}

#endif // MODULESTREETS_H



