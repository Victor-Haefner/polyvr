#ifndef MODULETREE_H
#define MODULETREE_H

#include "BaseModule.h"
#include "../OSM/OSMMapDB.h"
#include <iostream>
#include <string>
#include <OpenSG/OSGSimpleMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleTree : public BaseModule {
    private:
        //BuildingMaterials* matBuildings[3];
        OSMMapDB* mapDB = 0;
        vector<SimpleMaterialRecPtr> matTrees;
        int treeCounter;

    public:
        virtual string getName();

        string id;
        vector<Vec2f> positions;

        ModuleTree(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager);

        virtual void loadBbox(AreaBoundingBox* bbox);

        void physicalize(bool b);

        VRGeometryPtr makeTreeGeometry(Vec3f position, int treeNum);

        int getRandom(string id);

        virtual void unloadBbox(AreaBoundingBox* bbox);
};

OSG_END_NAMESPACE;

#endif // MODULETREE_H

