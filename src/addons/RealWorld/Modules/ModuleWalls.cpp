#include "ModuleWalls.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace OSG;

void ModuleWalls::loadBbox(AreaBoundingBox* bbox) {
    auto mapDB = RealWorld::get()->getDB();
    auto mc = RealWorld::get()->getCoordinator();
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    cout << "LOADING WALLS FOR " << bbox->str << "\n" << flush;

    for(OSMWay* way: osmMap->osmWays) {

        for(WallMaterial* mat: wallList) {

            if (way->tags[mat->k] == mat->v) {
                //if (meshes.count(way->id)) continue;
                // load Polygons from osmMap
                Wall* wall = new Wall(way->id);
                for(string nodeId: way->nodeRefs) {
                    OSMNode* node = osmMap->osmNodeMap[nodeId];
                    Vec2f pos = mc->realToWorld(Vec2f(node->lat, node->lon));
                    wall->positions.push_back(pos);
                }

                if (wall->positions.size() < 3) continue;

                // generate mesh
                VRGeometryPtr geom = makeWallGeometry(wall, mat);
                root->addChild(geom);
                //meshes[wall->id] = geom;
            }

        }
    }
}

void ModuleWalls::unloadBbox(AreaBoundingBox* bbox) {
    auto mapDB = RealWorld::get()->getDB();
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

//            BOOST_FOREACH(OSMWay* way, osmMap->osmWays) {
//
//                BOOST_FOREACH(WallMaterial* mat, wallList) {
//                    if (way->tags[mat->k] == mat->v) {
//                        if (meshes.count(way->id)) {
//                            VRGeometryPtr geom = meshes[way->id];
//                            meshes.erase(way->id);
//                            delete geom;
//                        }
//                    }
//
//                }
//            }
}

void ModuleWalls::physicalize(bool b) {
    //for (auto mesh : meshes);
}

ModuleWalls::ModuleWalls() : BaseModule("ModuleWall") {
    // create List with materials
    fillWallList();
}

void ModuleWalls::fillWallList() {
    //simple Wall Texture
    //addWall("buildingWall_red.png", "barrier", "wall"); //intesects with streets
    addWall("brick_red.jpg", "barrier", "ditch");
    addWall("Walls/Chipped_Bricks.png", "barrier", "fence", 0.1f, 0.5f);
    addWall("brick_red.jpg", "barrier", "guard_rail");
    addWall("hedge.jpg", "barrier", "hedge", 0.25f, 0.5f);
    addWall("brick_red.jpg", "barrier", "kerb");
    addWall("brick_red.jpg", "barrier", "retaining_wall");
}

void ModuleWalls::addWall(string texture, string key, string value, float width, float height) {
    auto world = RealWorld::get()->getWorld();
    WallMaterial* w = new WallMaterial();
    w->material = SimpleMaterial::create();
    w->material->addChunk(world->getTexture(texture));
    w->k = key;
    w->v = value;
    w->width = width;
    w->height = height;

    Config::createPhongShader(w->material);

    wallList.push_back(w);
}

void ModuleWalls::addWall(string texture, string key, string value){
    addWall(texture, key, value, 0.5, 1);
}

void ModuleWalls::addWallPart(Vec2f a1, Vec2f b1, Vec2f a2, Vec2f b2, Vec2f a3, Vec2f b3, GeometryData* gdWall, float width, float height){
    Vec2f _NULL;

    Vec2f intersectStart1;
    Vec2f intersectStart2;
    Vec2f intersectEnd1;
    Vec2f intersectEnd2;

    Vec2f ortho1;
    Vec2f ortho2 = getNOrtho(a2, b2)*width/2;
    Vec2f ortho3 = getNOrtho(a3, b3)*width/2;
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

Vec2f ModuleWalls::getNOrtho(Vec2f a, Vec2f b){
    Vec2f res = Vec2f(-(b-a).getValues()[1], (b-a).getValues()[0]);
    res.normalize(); //vector length = 1;
    return res;
}

Vec2f ModuleWalls::getIntersection(Vec2f a1, Vec2f b1, Vec2f a2, Vec2f b2){
    Vec2f _NULL;
    Vec2f v1 = a1-b1;
    Vec2f v2 = a2-b2;
    if(v1 == v2) return _NULL; //if parallel
    float t;
    float ax = a1.getValues()[0];
    float ay = a1.getValues()[1];
    float bx = v1.getValues()[0];
    float by = v1.getValues()[1];
    float cx = a2.getValues()[0];
    float cy = a2.getValues()[1];
    float dx = v2.getValues()[0];
    float dy = v2.getValues()[1];

    t = (by*ax + bx*cy - bx*ay - cx*by)/(dx*by - bx*dy);
    return Vec2f(cx + dx*t, cy + dy*t);
}

void ModuleWalls::createWallSide(Vec2f a, Vec2f b, Vec2f normal2D, GeometryData* gdWall, float height) {
    auto mc = RealWorld::get()->getCoordinator();
    Vec3f normal = Vec3f(normal2D.getValues()[0], 0, normal2D.getValues()[1]);
    normal.normalize();

    float length = (b-a).length();

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(normal);
    gdWall->pos->addValue(Vec3f(a.getValues()[0], mc->getElevation(a), a.getValues()[1]));
    gdWall->texs->addValue(Vec2f(0, 0));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(normal);
    gdWall->pos->addValue(Vec3f(b.getValues()[0], mc->getElevation(b), b.getValues()[1]));
    gdWall->texs->addValue(Vec2f(0, length));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(normal);
    gdWall->pos->addValue(Vec3f(b.getValues()[0], mc->getElevation(b) +  height, b.getValues()[1]));
    gdWall->texs->addValue(Vec2f(height, length));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(normal);
    gdWall->pos->addValue(Vec3f(a.getValues()[0], mc->getElevation(a) +  height, a.getValues()[1]));
    gdWall->texs->addValue(Vec2f(height, 0));
}

void ModuleWalls::createWallRoof(Vec2f a1, Vec2f a2, Vec2f b1, Vec2f b2, GeometryData* gdWall, float height) {
    auto mc = RealWorld::get()->getCoordinator();
    //float length = (a1 - a2).length();

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(Vec3f(0, 1, 0));
    gdWall->pos->addValue(Vec3f(a1.getValues()[0], mc->getElevation(a1) + height, a1.getValues()[1]));
    gdWall->texs->addValue(Vec2f(a1.getValues()[0], a1.getValues()[1]));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(Vec3f(0, 1, 0));
    gdWall->pos->addValue(Vec3f(a2.getValues()[0], mc->getElevation(a2) + height, a2.getValues()[1]));
    gdWall->texs->addValue(Vec2f(a2.getValues()[0], a2.getValues()[1]));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(Vec3f(0, 1, 0));
    gdWall->pos->addValue(Vec3f(b2.getValues()[0], mc->getElevation(b2) +  height, b2.getValues()[1]));
    gdWall->texs->addValue(Vec2f(b2.getValues()[0], b2.getValues()[1]));

    gdWall->inds->addValue(gdWall->inds->size());
    gdWall->norms->addValue(Vec3f(0, 1, 0));
    gdWall->pos->addValue(Vec3f(b1.getValues()[0], mc->getElevation(b1) +  height, b1.getValues()[1]));
    gdWall->texs->addValue(Vec2f(b1.getValues()[0], b1.getValues()[1]));
}

//to do
void ModuleWalls::addWall(Wall* wall, GeometryData* gdWall, float width, float height){
    //create AB todo
    Vec2f _NULL;
    vector<Vec2f*> sides = wall->getSides(); //get wall part

    //create wall part
    Vec2f* sidePrePre = NULL;
    Vec2f* sidePre = NULL;
    for(Vec2f* side: sides) {

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

VRGeometryPtr ModuleWalls::makeWallGeometry(Wall* wall, WallMaterial* wallMat) {
    GeometryData* gdWall = new GeometryData();

    addWall(wall, gdWall, wallMat->width, wallMat->height);

    VRGeometryPtr geomWall = VRGeometry::create(wallMat->v);

    geomWall->create(GL_QUADS, gdWall->pos, gdWall->norms, gdWall->inds, gdWall->texs);
    geomWall->setMaterial(wallMat->material);

    VRGeometryPtr geom = VRGeometry::create("Wall");
    if (gdWall->inds->size() > 0) geom->addChild(geomWall);
    return geom;
}
