#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <OpenSG/OSGVector.h>
#include "core/objects/object/VRObject.h"
#include "../MapCoordinator.h"
#include "../TextureManager.h"
#include <boost/format.hpp>

using namespace std;

namespace realworld {

struct AreaBoundingBox {
    OSG::Vec2f min;
    OSG::Vec2f max;
    string str;

    AreaBoundingBox(OSG::Vec2f min, float gridSize);
};

class BaseModule {
    public:
        virtual void loadBbox(AreaBoundingBox* bbox) = 0;
        virtual void unloadBbox(AreaBoundingBox* bbox) = 0;
        virtual string getName() = 0;
        virtual void physicalize(bool b) = 0;

        bool physicalized = false;
        OSG::VRObject* getRoot();

    protected:
        MapCoordinator* mapCoordinator;
        OSG::VRObject* root;
        TextureManager* texManager;

        BaseModule(MapCoordinator* mapCoordinator, TextureManager* texManager);
};

}

#endif // BASEMODULE_H
