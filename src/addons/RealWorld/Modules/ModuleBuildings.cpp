#include "ModuleBuildings.h"

#include "Building.h"
#include "../OSM/OSMMapDB.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "triangulate.h"
#include "../Config.h"
#include "../MapGeometryGenerator.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;
using namespace std;
using namespace realworld;

ModuleBuildings::ModuleBuildings(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
    this->mapDB = mapDB;

    b_mat = new VRMaterial("Buildings");
    b_mat->setTexture("world/textures/Buildings.png", false);
    b_mat->setAmbient(Color3f(0.7, 0.7, 0.7)); //light reflection in all directions
    b_mat->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
    b_mat->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction

    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    b_mat->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    b_mat->readFragmentShader(wdir+"/shader/TexturePhong/phong_building.fp"); //Fragment Shader
    b_mat->setMagMinFilter("GL_LINEAR", "GL_NEAREST_MIPMAP_NEAREST");

    b_geo_d = new GeometryData();
    r_geo_d = new GeometryData();
}

string ModuleBuildings::getName() { return "ModuleBuildings"; }

int ModuleBuildings::numberFromString(string s) {
    int hash = 0;
    int offset = 'a' - 1;
    for(string::const_iterator it=s.begin(); it!=s.end(); ++it) {
        hash = hash << 1 | (*it - offset);
    }
    if (hash < 0) hash = hash * (-1);
    return hash;
}

void ModuleBuildings::loadBbox(AreaBoundingBox* bbox) {
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    VRGeometry* b_geo = new VRGeometry("Buildings");
    VRGeometry* r_geo = new VRGeometry("Roofs");
    root->addChild(b_geo);
    root->addChild(r_geo);

    cout << "LOADING BUILDINGS FOR " << bbox->str << "\n" << flush;

    for(OSMWay* way : osmMap->osmWays) {
        if (way->tags["building"] != "yes") continue;
        //if (meshes.count(way->id)) continue;

        // load building from osmMap
        Building* b = new Building(way->id);
        for(string nodeId : way->nodeRefs) {
            OSMNode* node = osmMap->osmNodeMap[nodeId];
            Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
            b->positions.push_back(pos);
        }

        if (b->positions.size() < 3) continue;

        // generate mesh
        b->makeClockwise();
        makeBuildingGeometry(b);
    }

    b_geo->create(GL_QUADS, b_geo_d->pos, b_geo_d->norms, b_geo_d->inds, b_geo_d->texs);
    b_geo->setTexCoords(b_geo_d->texs2, 1);
    b_geo->setMaterial(b_mat);
    b_geos[bbox->str] = b_geo;
    b_geo_d->clear();

    r_geo->create(GL_TRIANGLES, r_geo_d->pos, r_geo_d->norms, r_geo_d->inds, r_geo_d->texs);
    //r_geo->setTexCoords(r_geo_d->texs2, 1);
    r_geo->setMaterial(b_mat);
    r_geos[bbox->str] = r_geo;
    r_geo_d->clear();

    b_geo->getPhysics()->setShape("Concave");
    b_geo->getPhysics()->setPhysicalized(physics);
}

void ModuleBuildings::unloadBbox(AreaBoundingBox* bbox) {
    string id = bbox->str;
    if (b_geos.count(id)) { b_geos[id]->destroy(); b_geos.erase(id); }
    if (r_geos.count(id)) { r_geos[id]->destroy(); r_geos.erase(id); }
}

void ModuleBuildings::physicalize(bool b) {
    physics = b;
    for (auto g : b_geos) {
        g.second->getPhysics()->setShape("Concave");
        g.second->getPhysics()->setPhysicalized(b);
    }
}

void ModuleBuildings::addBuildingWallLevel(Vec2f pos1, Vec2f pos2, int level, int bNum, float elevation) {
    srand(bNum); //seed for random windows
    float len = (pos2 - pos1).length();
    Vec2f wallDir = (pos2 - pos1);
    wallDir.normalize();

    float wall_segment = Config::get()->WINDOW_DOOR_WIDTH;
    float FLOOR_HEIGHT = Config::get()->BUILDING_FLOOR_HEIGHT;

    int segN = floor(len / wall_segment);
    segN = max(segN, 1);
    wall_segment = len / segN;

    float low = level * FLOOR_HEIGHT + elevation;
    float high = low + FLOOR_HEIGHT;

    // insert a door at a random place (when on level 0 && there is enough room)
    int doorIndex = -1;
    if (level == 0 && segN > 2) doorIndex = bNum % segN;

    int N = 4;
    float _N = 1./N;
    float e = 0.01;

    int di = N*float(rand()) / RAND_MAX;
    int wi = N*float(rand()) / RAND_MAX;
    int fi = N*float(rand()) / RAND_MAX;

    float d_tc1 = di * _N + e;
    float d_tc2 = di * _N - e + _N;
    float w_tc1 = wi * _N + e;
    float w_tc2 = wi * _N - e + _N;
    float f_tc1 = fi * _N + e;
    float f_tc2 = fi * _N - e + _N;

    for (int i=0; i<segN; i++) {
        Vec2f w1 = pos1 + (wallDir * (i*wall_segment));
        Vec2f w2 = pos1 + (wallDir * ((i+1)*wall_segment));

        Vec2f wallVector = w2-w1;
        Vec3f normal = Vec3f(-wallVector.getValues()[1], 0, wallVector.getValues()[0]);

        b_geo_d->pos->addValue(Vec3f(w1.getValues()[0], low, w1.getValues()[1]));
        b_geo_d->pos->addValue(Vec3f(w2.getValues()[0], low, w2.getValues()[1]));
        b_geo_d->pos->addValue(Vec3f(w2.getValues()[0], high, w2.getValues()[1]));
        b_geo_d->pos->addValue(Vec3f(w1.getValues()[0], high, w1.getValues()[1]));

        for (int k=0; k<4; k++) {
            b_geo_d->inds->addValue(b_geo_d->inds->size());
            b_geo_d->norms->addValue(normal);
        }

        if (i == doorIndex) { // door
            b_geo_d->texs2->addValue(Vec2f(d_tc1, 0.5+e));
            b_geo_d->texs2->addValue(Vec2f(d_tc2, 0.5+e));
            b_geo_d->texs2->addValue(Vec2f(d_tc2, 0.75-e));
            b_geo_d->texs2->addValue(Vec2f(d_tc1, 0.75-e));
        } else { // window
            b_geo_d->texs2->addValue(Vec2f(w_tc1, 0.25+e));
            b_geo_d->texs2->addValue(Vec2f(w_tc2, 0.25+e));
            b_geo_d->texs2->addValue(Vec2f(w_tc2, 0.5-e));
            b_geo_d->texs2->addValue(Vec2f(w_tc1, 0.5-e));
        }

        // wall
        b_geo_d->texs->addValue(Vec2f(f_tc1, e));
        b_geo_d->texs->addValue(Vec2f(f_tc2, e));
        b_geo_d->texs->addValue(Vec2f(f_tc2, 0.25-e));
        b_geo_d->texs->addValue(Vec2f(f_tc1, 0.25-e));
    }
}

Vec2f convRTC(float u, float v, Vec2f m) {
    Vec2f tc(u,v);
    tc -= m;
    float f = 0.003;
    tc *= f;
    tc += Vec2f(0.125, 0.875);
    //u *= f;
    //v *= f;
    //Vec2f tc(u-0.25*floor(4*u), 0.75+v-0.25*floor(4*v));
    //return tc;
    //cout << "AA " << Vec2f(u, v)-m << endl;
    return tc;
}

void ModuleBuildings::addBuildingRoof(Building* building, float height, float elevation){
    //create && fill vector a with polygon corners
    Vector2dVector a;
    bool first = true;
    for(auto corner : building->getCorners()) {
        if (first) { first=false; continue; }
        a.push_back( Vector2d(corner[0], corner[1]));
    }

    Vector2dVector result; // allocate an STL vector to hold the answer.
    Triangulate::Process(a,result); //  Invoke the triangulator to triangulate this polygon.

    //create roof
    height = (float)height * Config::get()->BUILDING_FLOOR_HEIGHT + elevation;
    int tcount = result.size()/3;
    for (int i=0; i<tcount; i++) {
        const Vector2d &p1 = result[i*3+0];
        const Vector2d &p2 = result[i*3+1];
        const Vector2d &p3 = result[i*3+2];

        //create triangle
        for (int j=0; j<3; j++) {
            r_geo_d->inds->addValue(r_geo_d->inds->size());
            r_geo_d->norms->addValue(Vec3f(0, 1, 0));
        }

        r_geo_d->pos->addValue(Vec3f(p1.GetX(), height, p1.GetY()));
        r_geo_d->pos->addValue(Vec3f(p2.GetX(), height, p2.GetY()));
        r_geo_d->pos->addValue(Vec3f(p3.GetX(), height, p3.GetY()));

        //r_geo_d->texs->addValue(Vec2f(p1.GetX()/factor, p1.GetY()/factor)); //use only border color of texture
        //r_geo_d->texs->addValue(Vec2f(p2.GetX()/factor, p2.GetY()/factor)); //use only border color of texture
        //r_geo_d->texs->addValue(Vec2f(p3.GetX()/factor, p3.GetY()/factor)); //use only border color of texture

        float mx = (p1.GetX() + p2.GetX() + p3.GetX() )/3.0;
        float my = (p1.GetY() + p2.GetY() + p3.GetY() )/3.0;

        r_geo_d->texs->addValue(convRTC(p1.GetX(), p1.GetY(), Vec2f(mx,my))); //use only border color of texture
        r_geo_d->texs->addValue(convRTC(p2.GetX(), p2.GetY(), Vec2f(mx,my))); //use only border color of texture
        r_geo_d->texs->addValue(convRTC(p3.GetX(), p3.GetY(), Vec2f(mx,my))); //use only border color of texture

        //r_geo_d->texs->addValue(Vec2f(0.0, 0.9)); //use only border color of texture
        //r_geo_d->texs->addValue(Vec2f(0.1, 0.9)); //use only border color of texture
        //r_geo_d->texs->addValue(Vec2f(0.0, 1.0)); //use only border color of texture
    }
}

/** create one Building **/
void ModuleBuildings::makeBuildingGeometry(Building* b) {
    int bNum = numberFromString(b->id);
    int height = bNum % Config::get()->MAX_FLOORS + 2;
    float minElevation = 99999.0f;

    for(auto corner : b->getCorners()){
        float cornerElevation = this->mapCoordinator->getElevation(corner);
        if(cornerElevation < minElevation) minElevation = cornerElevation;
    }

    for(auto side : b->getSides()) {
        for (int i=0; i<height; i++) {
            addBuildingWallLevel(side[0], side[1], i, bNum, minElevation);
        }
    }

    addBuildingRoof(b, height, minElevation);
}
