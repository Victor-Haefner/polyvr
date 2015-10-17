#include "MapGeometryGenerator.h"

#include "core/scene/VRScene.h"
#include "core/tools/VRText.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRTimer.h"
#include "World.h"
#include "StreetAlgos.h"
#include "TextureManager.h"

using namespace OSG;

GeometryData::GeometryData() { clear(); }

void GeometryData::clear() {
    pos = GeoPnt3fProperty::create();
    norms = GeoVec3fProperty::create();
    inds = GeoUInt32Property::create();
    texs = GeoVec2fProperty::create();
    texs2 = GeoVec2fProperty::create();
}


//MapGeometryGenerator(VRScene* scene, TextureManager* texManager) {
MapGeometryGenerator::MapGeometryGenerator(TextureManager* texManager) {
    //this->scene = scene;
    this->texManager = texManager;
}

void MapGeometryGenerator::updateWorldGeometry(World* world) {
    VRTimer t;
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

    t.stop();
    t.print();

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
