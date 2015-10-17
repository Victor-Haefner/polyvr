#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <OpenSG/OSGVector.h>
#include "core/objects/object/VRObject.h"
#include "../MapCoordinator.h"
#include "../TextureManager.h"
#include <boost/format.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

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
        OSG::VRObjectPtr getRoot();

    protected:
        MapCoordinator* mapCoordinator;
        OSG::VRObjectPtr root;
        TextureManager* texManager;

        BaseModule(MapCoordinator* mapCoordinator, TextureManager* texManager);
};

OSG_END_NAMESPACE;

#endif // BASEMODULE_H
