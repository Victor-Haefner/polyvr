#include "World.h"

#include "core/scene/VRScene.h"
#include "core/tools/VRText.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRTimer.h"
#include "World.h"
#include "StreetAlgos.h"

#include <OpenSG/OSGImage.h>

using namespace OSG;

/*GeometryData::GeometryData() { clear(); }

void GeometryData::clear() {
    pos = GeoPnt3fProperty::create();
    norms = GeoVec3fProperty::create();
    inds = GeoUInt32Property::create();
    texs = GeoVec2fProperty::create();
    texs2 = GeoVec2fProperty::create();
}*/

/*TextureObjChunkRecPtr World::getTexture(string key) {
    if (texMap.count(key)) return texMap[key];

    ImageRecPtr image = Image::create();
    image->read(("world/textures/"+key).c_str());
    TextureObjChunkRecPtr tex = TextureObjChunk::create();
    tex->setImage(image);
    texMap[key] = tex;
    return tex;
}*/

World::World() {
    /*ImageRecPtr imageStreetSegment = Image::create();
    imageStreetSegment->read("world/textures/street1.png");
    texStreetSegment = TextureObjChunk::create();
    texStreetSegment->setImage(imageStreetSegment);

    ImageRecPtr imageStreetJoint = Image::create();
    imageStreetJoint->read("world/textures/street1.png");
    texStreetJoint = TextureObjChunk::create();
    texStreetJoint->setImage(imageStreetJoint);

    ImageRecPtr imageSubQuad = Image::create();
    imageSubQuad->read("world/textures/asphalt.jpg");
    texSubQuad = TextureObjChunk::create();
    texSubQuad->setImage(imageSubQuad);*/

    //                ImageRecPtr img = Image::create();
    //                UChar8 data[] = {0,0,0, 255,50,50, 100,255,100, 255,255,255};
    //                img->set( Image::OSG_RGB_PF, 2, 2, 1, 1, 1, 0, data);
    //                texSubQuad = TextureObjChunk::create();
    //                texSubQuad->setImage(img);

    /*for(int i = 0; i<4; i++){
        TextureObjChunkRecPtr texTree;
        ImageRecPtr imageTree = Image::create();
        texTree = TextureObjChunk::create();
        stringstream ss;
        ss << i;
        string str = ss.str();
        imageTree->read(("world/textures/Tree/tree"+str+".png").c_str());
        texTree->setImage(imageTree);
        treeMapList.push_back(texTree);
    }*/
//                chunk->setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
//                chunk->setMagFilter( GL_LINEAR );
//                chunk->setWrapS( GL_CLAMP );//GL_CLAMP //GL_REPEAT
//                chunk->setWrapT( GL_CLAMP_TO_EDGE );//GL_CLAMP_TO_EDGE //GL_REPEAT

}

//vector<TextureObjChunkRecPtr> World::getTreeMap() { return treeMapList; }

void World::updateGeometry() {
    VRTimer t;
    t.start("mgg-unloading");

    // unload joints
    for (string jointId : listUnloadJointMeshes) {
        if (meshes.count(jointId)) meshes.erase(jointId);

        // unload segments attached to this joint
        StreetJoint* joint = streetJoints[jointId];
        for (auto seg : joint->segments) {
            if (meshes.count(seg->id)) meshes.erase(seg->id);
        }
    }

    // unload segments
    for (string segId : listUnloadSegmentMeshes) {
        if (meshes.count(segId)) meshes.erase(segId);
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
    listUnloadJointMeshes.clear();
    listLoadJointMeshes.clear();
    listUnloadSegmentMeshes.clear();
    listLoadSegmentMeshes.clear();
    listUnloadBuildingMeshes.clear();
    listLoadBuildingMeshes.clear();
    listLoadAreaMeshes.clear();
    listUnloadAreaMeshes.clear();
}
        /*1111111111111111
        void unloadMapData(MapData* mapData) {
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (meshes.count(j->id)) {
                    listUnloadJointMeshes.push_back(j->id);
                }
            }
            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                if (meshes.count(seg->id)) {
                    listUnloadSegmentMeshes.push_back(seg->id);
                }
            }
            BOOST_FOREACH(Building* b, mapData->buildings) {
                if (meshes.count(b->id)) {
                    listUnloadBuildingMeshes.push_back(b->id);
                }
            }
        }

        void removeMapData(MapData* mapData) {
            // TODO: deletes to free memory ??
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (streetJoints.count(j->id)) {
                    streetJoints.erase(j->id);
                }
            }
            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                if (streetSegments.count(seg->id)) {
                    streetSegments.erase(seg->id);
                }
            }
            BOOST_FOREACH(Building* b, mapData->buildings) {
                if (buildings.count(b->id)) {
                    buildings.erase(b->id);
                }
            }
        }
        void addMapData(MapData* mapData) {
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (this->streetJoints.find(j->id) == this->streetJoints.end()) {
                    this->streetJoints[j->id] = j;
                } else {
                    this->streetJoints[j->id]->merge(j);
                }
                if (meshes.count(j->id)) listUnloadJointMeshes.push_back(j->id);
                this->listLoadJointMeshes.push_back(j->id);
            }

            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                this->streetSegments[seg->id] = seg;
                if (meshes.count(seg->id)) listUnloadSegmentMeshes.push_back(seg->id);
                this->listLoadSegmentMeshes.push_back(seg->id);
            }

            // add buildings
            BOOST_FOREACH(Building* b, mapData->buildings) {
                this->buildings[b->id] = b;
                if (meshes.count(b->id)) listUnloadBuildingMeshes.push_back(b->id);
                this->listLoadBuildingMeshes.push_back(b->id);
            }

            // add areas
            this->listLoadAreaMeshes.push_back(mapData);
        }*/
