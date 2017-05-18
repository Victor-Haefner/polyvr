#include "ModuleTree.h"
#include "BaseWorldObject.h"
#include "../OSM/OSMNode.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../World.h"
#include "../MapCoordinator.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "addons/WorldGenerator/nature/VRTree.h"
#include <OpenSG/OSGTextureObjChunk.h>

using namespace OSG;

ModuleTree::ModuleTree(bool t, bool p) : BaseModule("ModuleTree", t,p) {}

void ModuleTree::loadBbox(MapGrid::Box bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    auto mapDB = RealWorld::get()->getDB();
    if (!mc || !mapDB) return;
    OSMMap* osmMap = mapDB->getMap(bbox.str);
    if (!osmMap) return;

    cout << "LOADING TREES FOR " << bbox.str << "\n" << flush;

    for (OSMNode* node : osmMap->osmNodes) {
        if(node->tags["natural"] != "tree") continue;

        Vec2f pos2D = mc->realToWorld(Vec2f(node->lat, node->lon));
        Vec3f pos3D = Vec3f(pos2D[0], mc->getElevation(pos2D), pos2D[1]);

        VRTreePtr tree = VRTree::create();
        tree->setup(4,4,rand(), 0.2,0.5,0.75,0.55, 0.2,0.5,0.2,0.2);
        tree->setFrom(pos3D);
        tree->setScale(Vec3f(2,2,2));
        root->addChild(tree);
        if (trees.count(bbox.str) == 0) trees[bbox.str] = vector<VRTreePtr>();
        trees[bbox.str].push_back(tree);
    }
}

void ModuleTree::unloadBbox(MapGrid::Box bbox) {
    if (trees.count(bbox.str)) {
        for (auto t : trees[bbox.str]) t->destroy();
        trees.erase(bbox.str);
    }
}

void ModuleTree::physicalize(bool b) {
    //for (auto mesh : meshes);
}


