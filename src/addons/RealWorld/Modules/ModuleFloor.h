#ifndef MODULEFLOOR_H
#define MODULEFLOOR_H

#include <map>
#include <OpenSG/OSGConfig.h>
#include "BaseModule.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoData;

class ModuleFloor : public BaseModule {
    private:
        VRMaterialPtr matSubquad;

        void makeFloor(Vec2f pointA, Vec2f pointB, VRGeoData& geo);
        void initMaterial();

    public:
        ModuleFloor(bool t, bool p);

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);

        void physicalize(bool b);
};

OSG_END_NAMESPACE;

#endif // MODULEFLOOR_H


