#include "ModuleFloor.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"

#include <GL/gl.h>
#include <GL/glut.h>

using namespace OSG;

ModuleFloor::ModuleFloor() : BaseModule("ModuleFloor") {
    // create material
    matSubquad = VRMaterial::create("ground");
    matSubquad->setTexture("world/textures/asphalt.jpg");
    matSubquad->setAmbient(Color3f(0.5, 0.5, 0.5));
    matSubquad->setDiffuse(Color3f(0.5, 0.6, 0.1));
    matSubquad->setSpecular(Color3f(0.2, 0.2, 0.2));
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    matSubquad->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    matSubquad->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");
    matSubquad->setMagMinFilter("GL_LINEAR", "GL_NEAREST_MIPMAP_NEAREST");
    matSubquad->setZOffset(1,1);

    //Config::createPhongShader(matSubquad);
    //matSubquad->addChunk(world->texSubQuad);
}

VRGeometryPtr ModuleFloor::makeSubQuadGeometry(Vec2f pointA, Vec2f pointB) {
    vector<Vec3f> pos;
    vector<Vec3f> norms;
    vector<int> inds;
    vector<Vec2f> texs;

    MapCoordinator* mapC = RealWorld::get()->getCoordinator();

    float x1 = pointA.getValues()[0];
    float y1 = pointA.getValues()[1];
    float x2 = pointB.getValues()[0];
    float y2 = pointB.getValues()[1];
    float tempX1, tempX2, tempY1, tempY2;

    int tesNum = 20;
    float deltaXStep = (x2 - x1)/(float)tesNum;
    float deltaYStep = (y2 - y1)/(float)tesNum;
    Vec3f v1Prev, v2Prev, v3Prev, v4Prev;
    for(int i = 0; i<tesNum; i++){
        for(int j = 0; j<tesNum; j++){
            tempX1 = x1 + i*deltaXStep;
            tempX2 = x1 + (i+1)*deltaXStep;
            tempY1 = y1 + j*deltaYStep;
            tempY2 = y1 + (j+1)*deltaYStep;
            Vec3f v1 = Vec3f(tempX1, mapC->getElevation(Vec2f(tempX1, tempY1)), tempY1);
            Vec3f v2 = Vec3f(tempX1, mapC->getElevation(Vec2f(tempX1, tempY2)), tempY2);
            Vec3f v3 = Vec3f(tempX2, mapC->getElevation(Vec2f(tempX2, tempY2)), tempY2);
            Vec3f v4 = Vec3f(tempX2, mapC->getElevation(Vec2f(tempX2, tempY1)), tempY1);

            Vec3f normal = MapCoordinator::getSurfaceNormal(v2-v1, v3-v1);

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v1);
            texs.push_back(Vec2f((x1-tempX1), (y1-tempY1)));

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v2);
            texs.push_back(Vec2f((x1-tempX1), (y1-tempY2)));

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v3);
            texs.push_back(Vec2f((x1-tempX2), (y1-tempY2)));

            normal = MapCoordinator::getSurfaceNormal(v3-v4, v4-v1);

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v3);
            texs.push_back(Vec2f((x1-tempX2), (y1-tempY2)));

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v4);
            texs.push_back(Vec2f((x1-tempX2), (y1-tempY1)));

            inds.push_back(inds.size());
            norms.push_back(normal);
            pos.push_back(v1);
            texs.push_back(Vec2f((x1-tempX1), (y1-tempY1)));

            v1Prev = v1; v2Prev = v2; v3Prev = v3; v4Prev = v4;
        }
    }

    VRGeometryPtr geom = VRGeometry::create("Subquad");
    geom->create(GL_TRIANGLES, pos, norms, inds, texs);
    return geom;
}

void ModuleFloor::loadBbox(MapGrid::Box bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    Vec2f min = mc->realToWorld(bbox.min);
    Vec2f max = mc->realToWorld(bbox.max);

    VRGeometryPtr geom = makeSubQuadGeometry(min, max);
    geom->setMaterial(matSubquad);
    root->addChild(geom);
    //this->scene->physicalize(geom, true);

    meshes[bbox.str] = geom;
    physicalize(physicalized);
}

void ModuleFloor::unloadBbox(MapGrid::Box bbox) {
    meshes.erase(bbox.str);
}

void ModuleFloor::physicalize(bool b) {
    physicalized = b;
    for (auto mesh : meshes) {
        mesh.second->getPhysics()->setPhysicalized(b);
        mesh.second->getPhysics()->setShape("Concave");
    }
}
