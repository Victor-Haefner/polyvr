#ifndef MODULEFLOOR_H
#define MODULEFLOOR_H

#include <map>
#include <OpenSG/OSGConfig.h>
#include "BaseModule.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class ModuleFloor : public BaseModule {
    private:
        VRMaterialPtr matSubquad;
        map<string, VRGeometryPtr> meshes;

        VRGeometryPtr makeSubQuadGeometry(Vec2f pointA, Vec2f pointB);

    public:
        ModuleFloor();

        virtual void loadBbox(AreaBoundingBox* bbox);
        virtual void unloadBbox(AreaBoundingBox* bbox);

        void physicalize(bool b);
};

OSG_END_NAMESPACE;

#endif // MODULEFLOOR_H


