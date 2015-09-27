#ifndef MAPGEOMETRYGENERATOR_H
#define	MAPGEOMETRYGENERATOR_H

#include "core/scene/VRScene.h"
#include "core/tools/VRText.h"
#include "core/objects/geometry/VRGeometry.h"
#include "World.h"
#include "StreetAlgos.h"
#include "TextureManager.h"
#include "Timer.h"

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;
using namespace std;

namespace realworld {
    struct GeometryData {
        GeoPnt3fPropertyRecPtr      pos;
        GeoVec3fPropertyRefPtr      norms;
        GeoUInt32PropertyRefPtr     inds;
        GeoVec2fPropertyRecPtr      texs;
        GeoVec2fPropertyRecPtr      texs2;

        GeometryData() {
            pos = GeoPnt3fProperty::create();
            norms = GeoVec3fProperty::create();
            inds = GeoUInt32Property::create();
            texs = GeoVec2fProperty::create();
            texs2 = GeoVec2fProperty::create();
        }

        void clear() {
            pos = GeoPnt3fProperty::create();
            norms = GeoVec3fProperty::create();
            inds = GeoUInt32Property::create();
            texs = GeoVec2fProperty::create();
            texs2 = GeoVec2fProperty::create();
        }

        /*vector<Vec3f> pos;
        vector<Vec3f> norms;
        vector<int> inds;
        vector<Vec2f> texs;
        vector<Vec2f> texs2;*/
    };

    class MapGeometryGenerator {
    public:
        //VRScene* scene;
        TextureManager* texManager;

        //MapGeometryGenerator(VRScene* scene, TextureManager* texManager) {
        MapGeometryGenerator(TextureManager* texManager) {
            //this->scene = scene;
            this->texManager = texManager;
        }

        void updateWorldGeometry(World* world) {
            Timer t;
            t.start("mgg-materials");
            t.printTime("mgg-materials");
            t.start("mgg-unloading");

            // unload joints
            for (string jointId : world->listUnloadJointMeshes) {
                if (world->meshes.count(jointId)) world->meshes.erase(jointId);

                // unload segments attached to this joint
                StreetJoint* joint = world->streetJoints[jointId];
                for (string segId : joint->segmentIds) {
                    if (world->meshes.count(segId)) world->meshes.erase(segId);
                }
            }

            // unload segments
            for (string segId : world->listUnloadSegmentMeshes) {
                if (world->meshes.count(segId)) world->meshes.erase(segId);
            }

            t.printTime("mgg-unloading");
            t.start("mgg-loading");
            t.printTime("mgg-loading");

            // test physics stuff
            /*VRGeometryPtr sphere;
            Vec3f center = Vec3f(20,10,20);
            sphere = VRGeometry::create("sphere");
            sphere->setMesh(makeSphereGeo(1,2));
            sphere->setFrom(center);
            sphere->setOrientation(center + Vec3f(0,1,0), Vec3f(1,0,0.5));
            scene->add(sphere);
            scene->physicalize(sphere, false);
            //scene->setGravity(Vec3f(0, -1000, 0));
            btRigidBody* rb = sphere->getBulletObject();
            rb->setLinearVelocity(btVector3(60, -100, 60));*/

            // cleanup
            world->listUnloadJointMeshes.clear();
            world->listLoadJointMeshes.clear();
            world->listUnloadSegmentMeshes.clear();
            world->listLoadSegmentMeshes.clear();
            world->listUnloadBuildingMeshes.clear();
            world->listLoadBuildingMeshes.clear();
            world->listLoadAreaMeshes.clear();
            world->listUnloadAreaMeshes.clear();
        }

    private:
    };
}

#endif	/* MAPGEOMETRYGENERATOR_H */

