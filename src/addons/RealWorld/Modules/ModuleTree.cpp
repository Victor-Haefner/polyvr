#include "ModuleTree.h"
#include "BaseWorldObject.h"
#include "../OSM/OSMNode.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../World.h"
#include "../MapCoordinator.h"
#include "core/objects/material/VRShader.h"
#include "addons/RealWorld/nature/VRTree.h"
#include <OpenSG/OSGTextureObjChunk.h>

using namespace OSG;

ModuleTree::ModuleTree() : BaseModule("ModuleTree") {
    this->treeCounter = 0;
    auto world = RealWorld::get()->getWorld();

    // Enable blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(TextureObjChunkRecPtr matTex : world->getTreeMap()){
        SimpleMaterialRecPtr mat;
        // create material
        mat = SimpleMaterial::create();
        //matSubquad->setDiffuse(Color3f(1, 1, 1));
        Config::createPhongShader(mat, false);
        mat->addChunk(matTex);
        matTrees.push_back(mat);
    }
}

void ModuleTree::loadBbox(AreaBoundingBox* bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    cout << "LOADING TREES FOR " << bbox->str << "\n" << flush;

    for (OSMNode* node : osmMap->osmNodes) {
            if(node->tags["natural"] == "tree"){
                Vec2f pos2D = mc->realToWorld(Vec2f(node->lat, node->lon));
                Vec3f pos3D = Vec3f(pos2D.getValues()[0], mc->getElevation(pos2D), pos2D.getValues()[1]);

                VRTreePtr tree = VRTree::create();
                tree->setFrom(pos3D);
                int treeNum = getRandom(node->id);

                // generate mesh
                VRGeometryPtr geom = makeTreeGeometry(pos3D, treeNum);
                root->addChild(geom);

                //meshes[wall->id] = geom;
            }
    }
}

void ModuleTree::physicalize(bool b) {
    //for (auto mesh : meshes);
}

VRGeometryPtr ModuleTree::makeTreeGeometry(Vec3f position, int treeNum) {
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



    VRGeometryPtr geomTree = VRGeometry::create("Tree");
    geomTree->create(GL_QUADS, pos, norms, inds, texs);
    geomTree->setMaterial(matTrees[treeNum]);
    return geomTree;
}

int ModuleTree::getRandom(string id) {
    int numb;
    istringstream ( id ) >> numb;
    numb %= matTrees.size();
    return numb;
}

void ModuleTree::unloadBbox(AreaBoundingBox* bbox) {
    //to do
}
