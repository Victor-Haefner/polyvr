#ifndef MODULETERRAIN_H
#define MODULETERRAIN_H

#include "Terrain.h"
#include "../OSM/OSMMapDB.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPhysics.h"
#include "triangulate.h"

using namespace OSG;
using namespace std;

namespace realworld {
    struct TerrainMaterial {
        VRMaterial* material;
        string k;
        string v;
        float height;
    };

    class ModuleTerrain: public BaseModule {
        public:
            virtual string getName() { return "ModuleTerrain"; }

            virtual void loadBbox(AreaBoundingBox* bbox) {
                OSMMap* osmMap = mapDB->getMap(bbox->str);
                if (!osmMap) return;

                cout << "LOADING TERRAINS FOR " << bbox->str << "\n" << flush;

                for (OSMWay* way : osmMap->osmWays) {
                    for (auto mat : terrainList) {
                        if (way->tags[mat->k] == mat->v) {
                            if (meshes.count(way->id)) continue;
                            // load Polygons from osmMap
                            Terrain* ter = new Terrain(way->id);
                            for (string nodeId : way->nodeRefs) {
                                OSMNode* node = osmMap->osmNodeMap[nodeId];
                                Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
                                ter->positions.push_back(pos);
                            }

                            if (ter->positions.size() < 3) continue;

                            // generate mesh
                            VRGeometry* geom = makeTerrainGeometry(ter, mat);
                            root->addChild(geom);
                            meshes[ter->id] = geom;
                        }

                    }
                }
        }

        virtual void unloadBbox(AreaBoundingBox* bbox) {
            OSMMap* osmMap = mapDB->getMap(bbox->str);
            if (!osmMap) return;
            for (OSMWay* way : osmMap->osmWays) {
                for (TerrainMaterial* mat : terrainList) {
                    if (way->tags[mat->k] == mat->v) {
                        if (meshes.count(way->id)) {
                            VRGeometry* geom = meshes[way->id];
                            meshes.erase(way->id);
                            delete geom;
                        }
                    }

                }
            }
        }

        void physicalize(bool b) {
            return;
            for (auto mesh : meshes) {
                mesh.second->getPhysics()->setPhysicalized(true);
                mesh.second->getPhysics()->setShape("Concave");
            }
        }

        ModuleTerrain(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
            this->mapDB = mapDB;

            // create List with materials
            fillTerrainList();
        }

    private:
        vector<TerrainMaterial*> terrainList;
        map<string, VRMaterial*> materials;
        //TerrainMaterials* matTerrain;
        OSMMapDB* mapDB;
        map<string, VRGeometry*> meshes;
        map<string, VRGeometry*>::iterator mesh_itr;

        void fillTerrainList() {
            //grass
            addTerrain("Terrain/hdGrass.jpg", "landuse", "grass");
            addTerrain("Terrain/hdGrass.jpg", "leisure", "common");
            addTerrain("Terrain/hdGrass.jpg", "leisure", "garden");
            addTerrain("Terrain/hdGrass.jpg", "leisure", "golf_course");
            addTerrain("Terrain/hdGrass.jpg", "landuse", "meadow");
            addTerrain("Terrain/hdGrass.jpg", "leisure", "park");
            addTerrain("Terrain/hdGrass.jpg", "leisure", "pitch");

            //water
            addTerrain("Terrain/water.jpg", "natural", "water", 3);

            //other
            addTerrain("Terrain/naturalMud.jpg", "natural", "mud");

            //from wiki, please replace with better textures
            addTerrain("wiki/landuseRecreation_ground.png", "landuse", "recreation_ground");
            addTerrain("wiki/landuseReservoir.png", "landuse", "reservoir");
            addTerrain("wiki/landuseRetail.png", "landuse", "retail");
            addTerrain("wiki/landuseVillage_green.png", "landuse", "village_green");
            addTerrain("wiki/landuseAllotments.png", "landuse", "allotments", 2);
            addTerrain("wiki/landuseBrownfield.png", "landuse", "brownfield", 2);
            addTerrain("wiki/landuseCemetery.png", "landuse", "cemetery", 2);
            addTerrain("wiki/landuseCommercial.png", "landuse", "commercial", 2);
            addTerrain("wiki/landuseConstruction.png", "landuse", "construction", 2);
            addTerrain("wiki/landuseFarm.png", "landuse", "farm", 2);
            addTerrain("wiki/landuseForest.png", "landuse", "forest", 2);
            addTerrain("wiki/landuseLandfill.png", "landuse", "landfill", 2);
            addTerrain("wiki/landuseMilitary.png", "landuse", "military", 2);
            addTerrain("wiki/landuseOrchard.png", "landuse", "orchard", 2);
            addTerrain("wiki/landusePlant_nursery.png", "landuse", "plant_nursery", 2);
            addTerrain("wiki/landuseVineyard.png", "landuse", "vineyard", 2);
            addTerrain("wiki/leisureNature_reserve.png", "leisure", "nature_reserve", 2);
            addTerrain("wiki/leisureSport_centre.png", "leisure", "sport_centre", 2);
            addTerrain("wiki/leisureStadium.png", "leisure", "stadium", 2);
            addTerrain("wiki/leisureTrack.png", "leisure", "track", 2);
            addTerrain("wiki/militaryDanger_area.png", "military", "danger_area", 2);
            addTerrain("wiki/naturalBeach.png", "natural", "beach", 2);
            addTerrain("wiki/naturalGrassland.png", "natural", "grassland", 2);
            addTerrain("wiki/naturalHeath.png", "natural", "heath", 2);
            addTerrain("wiki/naturalScrub.png", "natural", "scrub", 2);
            addTerrain("wiki/naturalWetland.png", "natural", "wetland", 2);
            addTerrain("wiki/naturalWood.png", "natural", "wood", 2);
            addTerrain("wiki/tourismZoo.png", "tourism", "zoo", 2);

            addTerrain("wiki/landuseBasin.png", "landuse", "basin", 3);
            addTerrain("wiki/leisurePlayground.png", "leisure", "playground", 3);
        }

        void addTerrain(string t, string k, string v){ addTerrain(t, k, v, 1); }
        void addTerrain(string texture, string key, string value, int height) {
            texture = "world/textures/"+texture;
            if (materials.count(texture) == 0) {
                materials[texture] = new VRMaterial(texture);
                materials[texture]->setTexture(texture);
                string wdir = VRSceneManager::get()->getOriginalWorkdir();
                materials[texture]->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
                materials[texture]->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");
                materials[texture]->setMagMinFilter("GL_LINEAR", "GL_NEAREST_MIPMAP_NEAREST");
            }

            TerrainMaterial* m = new TerrainMaterial();
            m->material = materials[texture];
            m->k = key;
            m->v = value;
            m->height = height;
            terrainList.push_back(m);
        }

        void addTerrain(Terrain* ter, GeometryData* gdTerrain, int height){
            //create && fill vector a with polygon corners
            Vector2dVector a;
            bool first = true;
            for (Vec2f corner : ter->getCorners()) {
                if(first){ first=false; continue;} //first && last corners are equal, so ignoring first one
                 a.push_back( Vector2d(corner[0], corner[1]));
            }

            // allocate an STL vector to hold the answer.
            Vector2dVector result;

            //  Invoke the triangulator to triangulate this polygon.
            Triangulate::Process(a,result);

            int tcount = result.size()/3;

            //reference values to map the texture on the triangles
            //float refX = result[0].GetX();
            //float refY = result[0].GetY();

            //float scale = this->mapCoordinator->SCALE_REAL_TO_WORLD;

            for (int i=0; i<tcount; i++) {
                const Vector2d &p1 = result[i*3+0];
                const Vector2d &p2 = result[i*3+1];
                const Vector2d &p3 = result[i*3+2];

                //compute the hight. Smaller areas are higher, otherwise you could not see them.
                //const float area = Triangulate::Area(result);
                //int height = 1;
                /*if(area < 1000) height = 7;
                else if(area < 5000) height = 6;
                else if(area < 10000) height = 5;
                else if(area < 50000) height = 4;
                else if(area < 100000) height = 3;
                else if(area < 300000) height = 2;
                //if(height > 10) height = 10;*/

                float groundLevel = (float)height * Config::get()->LAYER_DISTANCE ;

                //create triangle
                tesselateTriangle(p1.GetX(), p1.GetY(), p2.GetX(), p2.GetY(), p3.GetX(), p3.GetY(), groundLevel, gdTerrain);
              }
        }

        Vec3f tesselateTriangle(float p1X, float p1Y, float p2X, float p2Y, float p3X, float p3Y, float height, GeometryData* gdTerrain){
            return tesselateTriangle(p1X, p1Y, p2X, p2Y, p3X, p3Y, height, gdTerrain, Vec3f(0,0,0));
        }

        Vec3f tesselateTriangle(float p1X, float p1Y, float p2X, float p2Y, float p3X, float p3Y, float height, GeometryData* gdTerrain, Vec3f usedCorners){
            Vec3f res = Vec3f(0, 0, 0);
            Vec2f p1 = Vec2f(p1X, p1Y);
            Vec2f p2 = Vec2f(p2X, p2Y);
            Vec2f p3 = Vec2f(p3X, p3Y);
            if (p1==p2 || p2==p3 || p1==p3) return res;
            if ((p1-p2).length() > Config::get()->maxTriangleSize && (p1-p2).length() > (p1-p3).length() && (p1-p2).length() > (p2-p3).length()){
                Vec2f p12 = p1 + (p2-p1)/2;
                float p12X = p12.getValues()[0];
                float p12Y = p12.getValues()[1];
                res = tesselateTriangle(p1X, p1Y, p12X, p12Y, p3X, p3Y, height, gdTerrain);
                tesselateTriangle(p12X, p12Y, p2X, p2Y, p3X, p3Y, height, gdTerrain, Vec3f(res.getValues()[1], 0, res.getValues()[2]));
                return Vec3f(0,0,0);
            } else if((p1-p3).length() > Config::get()->maxTriangleSize && (p1-p3).length() > (p2-p3).length()){
                Vec2f p13 = p1 + (p3-p1)/2;
                float p13X = p13.getValues()[0];
                float p13Y = p13.getValues()[1];
                res = tesselateTriangle(p1X, p1Y, p2X, p2Y, p13X, p13Y, height, gdTerrain);
                tesselateTriangle(p13X, p13Y, p2X, p2Y, p3X, p3Y, height, gdTerrain, Vec3f(res.getValues()[2], res.getValues()[1], 0));
                return Vec3f(0,0,0);
            } else if((p2-p3).length() > Config::get()->maxTriangleSize){
                Vec2f p23 = p2 + (p3-p2)/2;
                float p23X = p23.getValues()[0];
                float p23Y = p23.getValues()[1];
                res = tesselateTriangle(p1X, p1Y, p2X, p2Y, p23X, p23Y, height, gdTerrain);
                tesselateTriangle(p1X, p1Y, p23X, p23Y, p3X, p3Y, height, gdTerrain, Vec3f(res.getValues()[0], 0, res.getValues()[2]));
                return Vec3f(0,0,0);
            } else { //only if triangle is small enough, create geometry for it

                gdTerrain->norms->addValue(Vec3f(0, 1, 0));
                gdTerrain->pos->addValue(Vec3f(p1X, this->mapCoordinator->getElevation(p1X,p1Y) + height, p1Y)); // + getHight(p1.GetX(), p1.GetY())
                gdTerrain->texs->addValue(p1/5);
                gdTerrain->inds->addValue(gdTerrain->inds->size());

                gdTerrain->norms->addValue(Vec3f(0, 1, 0));
                gdTerrain->pos->addValue(Vec3f(p2X, this->mapCoordinator->getElevation(p2X, p2Y) + height, p2Y));
                gdTerrain->texs->addValue(p2/5);
                gdTerrain->inds->addValue(gdTerrain->inds->size());

                gdTerrain->norms->addValue(Vec3f(0, 1, 0));
                gdTerrain->pos->addValue(Vec3f(p3X, this->mapCoordinator->getElevation(p3X, p3Y) + height, p3Y));
                gdTerrain->texs->addValue(p3/5);
                gdTerrain->inds->addValue(gdTerrain->inds->size());

                return Vec3f(gdTerrain->inds->size()-3, gdTerrain->inds->size()-2, gdTerrain->inds->size()-1);
            }

            return Vec3f(0,0,0);
        }

        VRGeometry* makeTerrainGeometry(Terrain* ter, TerrainMaterial* terrainMat) {
            GeometryData* gdTerrain = new GeometryData();
            addTerrain(ter, gdTerrain, terrainMat->height);
            VRGeometry* geomTerrain = new VRGeometry(terrainMat->v);

            geomTerrain->create(GL_TRIANGLES, gdTerrain->pos, gdTerrain->norms, gdTerrain->inds, gdTerrain->texs);
            geomTerrain->setMaterial(terrainMat->material);

            VRGeometry* geom = new VRGeometry("Terrain");
            if (gdTerrain->inds->size() > 0) geom->addChild(geomTerrain);
            return geom;
        }

    };
}

#endif // MODULETERRAIN_H

