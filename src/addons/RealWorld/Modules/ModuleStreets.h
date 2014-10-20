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


            VRGeometry* makeSignGeometry(StreetSegment* seg);
            VRGeometry* makeStreetSegmentGeometry(StreetSegment* s);
            Vec3f getNormal3D(Vec3f v1, Vec3f v2);


            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, int* i,
                          vector<Vec3f>* pos, vector<Vec3f>* norms, vector<int>* inds, vector<Vec2f>* texs);
            void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, int* i,
                          vector<Vec3f>* pos, vector<Vec3f>* norms, vector<int>* inds, vector<Vec2f>* texs, bool isSide);
            VRGeometry* makeStreetJointGeometry(StreetJoint* sj);
    };
}

#endif // MODULESTREETS_H



