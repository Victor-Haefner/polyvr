#ifndef MODULETREE_H
#define MODULETREE_H

#include "BaseWorldObject.h"
#include "../OSM/OSMMapDB.h"
#include "core/objects/material/VRShader.h"
#include "addons/RealWorld/nature/VRTree.h"
#include <iostream>
#include <string>

namespace realworld {

    class ModuleTree: public BaseModule
    {
        private:
            //BuildingMaterials* matBuildings[3];
            OSMMapDB* mapDB;
            vector<SimpleMaterialRecPtr> matTrees;
            int treeCounter;

        public:
            virtual string getName() { return "ModuleTree"; }

            string id;
            vector<Vec2f> positions;

            ModuleTree(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
                this->mapDB = mapDB;
                this->treeCounter = 0;

                // Enable blending
                //glEnable(GL_BLEND);
                //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable (GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                BOOST_FOREACH(TextureObjChunkRecPtr matTex, texManager->treeMapList){
                    SimpleMaterialRecPtr mat;
                    // create material
                    mat = SimpleMaterial::create();
                    //matSubquad->setDiffuse(Color3f(1, 1, 1));
                    Config::createPhongShader(mat, false);
                    mat->addChunk(matTex);
                    matTrees.push_back(mat);
                }
            }

            virtual void loadBbox(AreaBoundingBox* bbox){
                OSMMap* osmMap = mapDB->getMap(bbox->str);
                if (!osmMap) return;

                cout << "LOADING TREES FOR " << bbox->str << "\n" << flush;

                BOOST_FOREACH(OSMNode* node, osmMap->osmNodes) {
                        if(node->tags["natural"] == "tree"){
                            Vec2f pos2D = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
                            Vec3f pos3D = Vec3f(pos2D.getValues()[0], this->mapCoordinator->getElevation(pos2D), pos2D.getValues()[1]);

                            VRTree* tree = new VRTree();
                            tree->setFrom(pos3D);
                            int treeNum = getRandom(node->id);

                            // generate mesh
                            VRGeometry* geom = makeTreeGeometry(pos3D, treeNum);
                            root->addChild(geom);

                            //meshes[wall->id] = geom;
                        }
                }
            }

        void physicalize(bool b) {
            //for (auto mesh : meshes);
        }

        VRGeometry* makeTreeGeometry(Vec3f position, int treeNum) {
            vector<Vec3f> pos;
            vector<Vec3f> norms;
            vector<int> inds;
            vector<Vec2f> texs;
            float width = Config::get()->TREE_WIDTH/2;
            float height = Config::get()->TREE_HEIGHT;

            pos.push_back(position + Vec3f(-width, 0.0f, 0.0f));
            pos.push_back(position + Vec3f(width, 0.0f, 0.0f));
            pos.push_back(position + Vec3f(width, height, 0.0f));
            pos.push_back(position + Vec3f(-width, height, 0.0f));

            pos.push_back(position + Vec3f(0.0f, 0.0f, -width));
            pos.push_back(position + Vec3f(0.0f, 0.0f, width));
            pos.push_back(position + Vec3f(0.0f, height, width));
            pos.push_back(position + Vec3f(0.0f, height, -width));

            inds.push_back(inds.size()); inds.push_back(inds.size()); inds.push_back(inds.size()); inds.push_back(inds.size());
            inds.push_back(inds.size()); inds.push_back(inds.size()); inds.push_back(inds.size()); inds.push_back(inds.size());

            texs.push_back(Vec2f(0.0f, 0.0f)); texs.push_back(Vec2f(1.0f, 0.0f)); texs.push_back(Vec2f(1.0f, 1.0f)); texs.push_back(Vec2f(0.0f, 1.0f));
            texs.push_back(Vec2f(0.0f, 0.0f)); texs.push_back(Vec2f(1.0f, 0.0f)); texs.push_back(Vec2f(1.0f, 1.0f)); texs.push_back(Vec2f(0.0f, 1.0f));

            norms.push_back(Vec3f(0.0f, 0.0f, 1.0f)); norms.push_back(Vec3f(0.0f, 0.0f, 1.0f)); norms.push_back(Vec3f(0.0f, 0.0f, 1.0f)); norms.push_back(Vec3f(0.0f, 0.0f, 1.0f));
            norms.push_back(Vec3f(1.0f, 0.0f, 0.0f)); norms.push_back(Vec3f(1.0f, 0.0f, 0.0f)); norms.push_back(Vec3f(1.0f, 0.0f, 0.0f)); norms.push_back(Vec3f(1.0f, 0.0f, 0.0f));



            VRGeometry* geomTree = new VRGeometry("Tree");
            geomTree->create(GL_QUADS, pos, norms, inds, texs);
            geomTree->setMaterial(matTrees[treeNum]);
            return geomTree;
        }

        int getRandom(string id){
            int numb;
            istringstream ( id ) >> numb;
            numb %= matTrees.size();
            return numb;
        }

            virtual void unloadBbox(AreaBoundingBox* bbox) {
                    //to do
            }



    };
}

#endif // MODULETREE_H

