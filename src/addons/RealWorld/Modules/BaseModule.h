#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <map>
#include <OpenSG/OSGVector.h>
#include "../MapGrid.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class BaseModule {
    public:
        bool useThreads = false;

    protected:
        string name;
        VRObjectPtr root;
        bool doPhysicalize = false;

        map<string, VRGeometryPtr> meshes;

    public:
        BaseModule(string name, bool t, bool p);

        string getName();
        VRObjectPtr getRoot();

        virtual void loadBbox(MapGrid::Box bbox) = 0;
        virtual void unloadBbox(MapGrid::Box bbox) = 0;
};

OSG_END_NAMESPACE;

#endif // BASEMODULE_H
