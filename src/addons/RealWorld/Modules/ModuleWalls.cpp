#include "ModuleWalls.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"

#include "addons/WorldGenerator/GIS/OSMMap.h"
#include "triangulate.h"
#include "Wall.h"

using namespace OSG;

void ModuleWalls::loadBbox(MapGrid::Box bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    OSMMapPtr osmMap = RealWorld::get()->getMap(bbox.str);
    if (!osmMap) return;

    cout << "LOADING WALLS FOR " << bbox.str << "\n" << flush;
    VRGeoData geo;

    for (auto way : osmMap->getWays()) {
        for (WallMaterial* mat : wallList) {
            if (way.second->tags[mat->k] == mat->v) {
                Wall* wall = new Wall(way.second->id);
                for(string nID: way.second->nodes) {
                    auto node = osmMap->getNode(nID);
                    Vec2d pos = mc->realToWorld(Vec2d(node->lat, node->lon));
                    wall->positions.push_back(pos);
                }

                if (wall->positions.size() < 3) continue;
                addWall(wall, geo, mat->width, mat->height);
            }
        }
    }

    VRGeometryPtr geom = geo.asGeometry("Wall");
    geom->setMaterial(wallList[0]->material);
    root->addChild(geom);
    meshes[bbox.str] = geom;
}

void ModuleWalls::unloadBbox(MapGrid::Box bbox) {
    meshes[bbox.str]->destroy();
    meshes.erase(bbox.str);
}

void ModuleWalls::physicalize(bool b) {}

ModuleWalls::ModuleWalls(bool t, bool p) : BaseModule("ModuleWall", t,p) {
    fillWallList(); // create List with materials
}

void ModuleWalls::fillWallList() {
    //simple Wall Texture
    //addWall("world/textures/buildingWall_red.png", "barrier", "wall"); //intesects with streets
    addWall("world/textures/brick_red.jpg", "barrier", "ditch");
    addWall("world/textures/Walls/Chipped_Bricks.png", "barrier", "fence", 0.1f, 0.5f);
    addWall("world/textures/brick_red.jpg", "barrier", "guard_rail");
    addWall("world/textures/hedge.jpg", "barrier", "hedge", 0.25f, 0.5f);
    addWall("world/textures/brick_red.jpg", "barrier", "kerb");
    addWall("world/textures/brick_red.jpg", "barrier", "retaining_wall");
}

void ModuleWalls::addWall(string texture, string key, string value, float width, float height) {
    WallMaterial* w = new WallMaterial();
    w->material = VRMaterial::create("wall");
    w->material->setTexture(texture);
    w->k = key;
    w->v = value;
    w->width = width;
    w->height = height;
    wallList.push_back(w);
}

void ModuleWalls::addWall(string texture, string key, string value){
    addWall(texture, key, value, 0.5, 1);
}

void ModuleWalls::addWallPart(Vec2d a1, Vec2d b1, Vec2d a2, Vec2d b2, Vec2d a3, Vec2d b3, VRGeoData& gdWall, float width, float height){
    Vec2d _NULL;

    Vec2d intersectStart1;
    Vec2d intersectStart2;
    Vec2d intersectEnd1;
    Vec2d intersectEnd2;

    Vec2d ortho1;
    Vec2d ortho2 = getNOrtho(a2, b2)*width/2;
    Vec2d ortho3 = getNOrtho(a3, b3)*width/2;
    if(a1 == _NULL || b1 == _NULL){
        intersectStart1 = a2 - ortho2;
        intersectStart2 = a2 + ortho2;
        createWallSide(intersectStart1, intersectStart2, a2-b2, gdWall, height); //wall start
    }else{
        ortho1 = getNOrtho(a1, b1)*width/2;

        intersectStart1 = getIntersection(a1-ortho1, b1-ortho1, a2-ortho2, b2-ortho2);
        intersectStart2 = getIntersection(a1+ortho1, b1+ortho1, a2+ortho2, b2+ortho2);
    }

    if(a3 == _NULL || b3 == _NULL){
        intersectEnd1 = b2 -ortho2;
        intersectEnd2 = b2 + ortho2;
        createWallSide(intersectEnd1, intersectEnd2, b2-a2, gdWall, height); //wall end
    }else{
        intersectEnd1 = getIntersection(a2-ortho2, b2-ortho2, a3-ortho3, b3-ortho3);
        intersectEnd2 = getIntersection(a2+ortho2, b2+ortho2, a3+ortho3, b3+ortho3);
    }
    if(intersectStart1 == _NULL || intersectEnd1 == _NULL) return; //if parallel
    else {
        createWallSide(intersectStart1, intersectEnd1, -ortho2, gdWall, height);
        createWallSide(intersectStart2, intersectEnd2, ortho2, gdWall, height);
        createWallRoof(intersectStart1, intersectStart2, intersectEnd1, intersectEnd2, gdWall, height);
    }
}

Vec2d ModuleWalls::getNOrtho(Vec2d a, Vec2d b){
    Vec2d res = Vec2d(-(b-a)[1], (b-a)[0]);
    res.normalize(); //vector length = 1;
    return res;
}

Vec2d ModuleWalls::getIntersection(Vec2d a1, Vec2d b1, Vec2d a2, Vec2d b2){
    Vec2d _NULL;
    Vec2d v1 = a1-b1;
    Vec2d v2 = a2-b2;
    if(v1 == v2) return _NULL; //if parallel
    float t;
    float ax = a1[0];
    float ay = a1[1];
    float bx = v1[0];
    float by = v1[1];
    float cx = a2[0];
    float cy = a2[1];
    float dx = v2[0];
    float dy = v2[1];

    t = (by*ax + bx*cy - bx*ay - cx*by)/(dx*by - bx*dy);
    return Vec2d(cx + dx*t, cy + dy*t);
}

void ModuleWalls::createWallSide(Vec2d a, Vec2d b, Vec2d normal2D, VRGeoData& gdWall, float height) {
    auto mc = RealWorld::get()->getCoordinator();
    Vec3d n = Vec3d(normal2D[0], 0, normal2D[1]);
    n.normalize();

    float Ha = mc->getElevation(a);
    float Hb = mc->getElevation(b);
    float length = (b-a).length();

    int Va = gdWall.pushVert(Vec3d(a[0], Ha, a[1]), n, Vec2d(0, 0));
    int Vb = gdWall.pushVert(Vec3d(b[0], Hb, b[1]), n, Vec2d(0, length));
    int Vc = gdWall.pushVert(Vec3d(b[0], Hb+height, b[1]), n, Vec2d(height, length));
    int Vd = gdWall.pushVert(Vec3d(a[0], Ha+height, a[1]), n, Vec2d(height, 0));
    gdWall.pushQuad(Va, Vb, Vc, Vd);
}

void ModuleWalls::createWallRoof(Vec2d a1, Vec2d a2, Vec2d b1, Vec2d b2, VRGeoData& gdWall, float height) {
    auto mc = RealWorld::get()->getCoordinator();
    Vec3d n = Vec3d(0, 1, 0);
    int Va = gdWall.pushVert(Vec3d(a1[0], mc->getElevation(a1) + height, a1[1]), n, Vec2d(a1[0], a1[1]));
    int Vb = gdWall.pushVert(Vec3d(a2[0], mc->getElevation(a2) + height, a2[1]), n, Vec2d(a2[0], a2[1]));
    int Vc = gdWall.pushVert(Vec3d(b2[0], mc->getElevation(b2) + height, b2[1]), n, Vec2d(b2[0], b2[1]));
    int Vd = gdWall.pushVert(Vec3d(b1[0], mc->getElevation(b1) + height, b1[1]), n, Vec2d(b1[0], b1[1]));
    gdWall.pushQuad(Va, Vb, Vc, Vd);
}

//to do
void ModuleWalls::addWall(Wall* wall, VRGeoData& gdWall, float width, float height){
    //create AB todo
    Vec2d _NULL;
    vector<Vec2d*> sides = wall->getSides(); //get wall part

    //create wall part
    Vec2d* sidePrePre = NULL;
    Vec2d* sidePre = NULL;
    for(Vec2d* side: sides) {

        if(sidePrePre != NULL){
            addWallPart(sidePrePre[0], sidePrePre[1], sidePre[0], sidePre[1], side[0], side[1], gdWall, width, height);
        }else if(sidePre != NULL){
            addWallPart(_NULL, _NULL, sidePre[0], sidePre[1], side[0], side[1], gdWall, width, height);
        }else{
            sidePre = side;
            continue;
        }
        sidePrePre = sidePre;
        sidePre = side;
    }
    addWallPart(sidePrePre[0], sidePrePre[1], sidePre[0], sidePre[1], _NULL, _NULL, gdWall, width, height);
}
