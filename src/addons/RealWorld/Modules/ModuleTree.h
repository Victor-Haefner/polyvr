#ifndef MODULETREE_H
#define MODULETREE_H

#include "BaseModule.h"
#include "../OSM/OSMMapDB.h"
#include <iostream>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleTree : public BaseModule {
    private:
        VRMaterialPtr mat;
        map<string, vector<VRTreePtr> > trees;

    public:
        string id;
        vector<Vec2f> positions;

        ModuleTree(bool t, bool p);

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);

        VRGeometryPtr makeTree(Vec3f position, int treeNum);

        int getRandom(string id);
};

OSG_END_NAMESPACE;

#endif // MODULETREE_H

