#include "ModuleFloor.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"

#include <GL/gl.h>
#include <GL/glut.h>

using namespace OSG;

ModuleFloor::ModuleFloor(bool t, bool p) : BaseModule("ModuleFloor", t,p) {
    initMaterial();
}

void ModuleFloor::makeFloor(Vec2f pointA, Vec2f pointB, VRGeoData& geo) {
    MapCoordinator* mapC = RealWorld::get()->getCoordinator();
    if (!mapC) return;

    float x1 = pointA[0];
    float y1 = pointA[1];
    float x2 = pointB[0];
    float y2 = pointB[1];
    float tempX1, tempX2, tempY1, tempY2;

    int tesNum = 20;
    float deltaXStep = (x2 - x1)/(float)tesNum;
    float deltaYStep = (y2 - y1)/(float)tesNum;
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

            Vec3f normal = (v2-v1).cross(v3-v1); // TODO: use quads ?
            normal = normal[1] < 0 ? -normal : normal; // face upwards!
            geo.pushVert(v1, normal, Vec2f(x1-tempX1, y1-tempY1));
            geo.pushVert(v2, normal, Vec2f(x1-tempX1, y1-tempY2));
            geo.pushVert(v3, normal, Vec2f(x1-tempX2, y1-tempY2));
            geo.pushTri();

            normal = (v3-v4).cross(v4-v1); // TODO: use quads ?
            normal = normal[1] < 0 ? -normal : normal; // face upwards!
            geo.pushVert(v3, normal, Vec2f(x1-tempX2, y1-tempY2));
            geo.pushVert(v4, normal, Vec2f(x1-tempX2, y1-tempY1));
            geo.pushVert(v1, normal, Vec2f(x1-tempX1, y1-tempY1));
            geo.pushTri();
        }
    }
}

void ModuleFloor::initMaterial() {
    // create material
    matSubquad = VRMaterial::create("ground");
    matSubquad->setTexture("world/textures/asphalt.jpg");
    matSubquad->setAmbient(Color3f(0.5, 0.5, 0.5));
    matSubquad->setDiffuse(Color3f(0.5, 0.6, 0.1));
    matSubquad->setSpecular(Color3f(0.2, 0.2, 0.2));
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    matSubquad->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    matSubquad->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");
    matSubquad->setMagMinFilter(GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 0);
    matSubquad->setZOffset(1,1);
}

void ModuleFloor::loadBbox(MapGrid::Box bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    if (!mc) return;
    Vec2f min = mc->realToWorld(bbox.min);
    Vec2f max = mc->realToWorld(bbox.max);

    VRGeoData geo;
    makeFloor(min, max, geo);
    VRGeometryPtr geom = geo.asGeometry("Subquad");
    geom->addAttachment("ground", 0);
    geom->setMaterial(matSubquad);
    root->addChild(geom);

    meshes[bbox.str] = geom;
    if (doPhysicalize) {
        geom->getPhysics()->setShape("Concave");
        geom->getPhysics()->setPhysicalized(true);
    }
}

void ModuleFloor::unloadBbox(MapGrid::Box bbox) {
    meshes[bbox.str]->destroy();
    meshes.erase(bbox.str);
}
