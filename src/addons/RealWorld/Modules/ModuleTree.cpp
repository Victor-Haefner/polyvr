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
#include "addons/RealWorld/nature/VRTree.h"
#include <OpenSG/OSGTextureObjChunk.h>

using namespace OSG;

ImageRecPtr treeTex;

ModuleTree::ModuleTree() : BaseModule("ModuleTree") {
	mat = VRMaterial::create("tree");
	VRTextureGenerator tg;
	tg.setSize(Vec3i(50,50,1));
	tg.add("Perlin", 1, Vec3f(0.7,0.5,0.3), Vec3f(1,0.9,0.7));
	tg.add("Perlin", 0.25, Vec3f(1,0.9,0.7), Vec3f(0.7,0.5,0.3));
	mat->setTexture(tg.compose(0));
	treeTex = tg.compose(0);
}

void ModuleTree::loadBbox(AreaBoundingBox* bbox) {
    auto mc = RealWorld::get()->getCoordinator();
    auto mapDB = RealWorld::get()->getDB();
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    cout << "LOADING TREES FOR " << bbox->str << "\n" << flush;

    for (OSMNode* node : osmMap->osmNodes) {
        if(node->tags["natural"] != "tree") continue;

        Vec2f pos2D = mc->realToWorld(Vec2f(node->lat, node->lon));
        Vec3f pos3D = Vec3f(pos2D[0], mc->getElevation(pos2D), pos2D[1]);

        VRTreePtr tree = VRTree::create();
        tree->setup(4,4,rand(), 0.2,0.5,0.75,0.55, 0.2,0.5,0.2,0.2);
        tree->setFrom(pos3D);
        root->addChild(tree);
        if (trees.count(bbox->str) == 0) trees[bbox->str] = vector<VRTreePtr>();
        trees[bbox->str].push_back(tree);

        tree->getMaterial()->setTexture(treeTex);
    }
}

void ModuleTree::unloadBbox(AreaBoundingBox* bbox) {
    if (trees.count(bbox->str)) {
        for (auto t : trees[bbox->str]) t->destroy();
        trees.erase(bbox->str);
    }
}

void ModuleTree::physicalize(bool b) {
    //for (auto mesh : meshes);
}


