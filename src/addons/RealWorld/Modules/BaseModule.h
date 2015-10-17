#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct AreaBoundingBox {
    Vec2f min;
    Vec2f max;
    string str;

    AreaBoundingBox(Vec2f min, float gridSize);
};

class BaseModule {
    protected:
        string name;
        VRObjectPtr root;
        bool physicalized = false;

    public:
        BaseModule(string name);

        string getName();
        VRObjectPtr getRoot();

        virtual void loadBbox(AreaBoundingBox* bbox) = 0;
        virtual void unloadBbox(AreaBoundingBox* bbox) = 0;
        virtual void physicalize(bool b) = 0;
};

OSG_END_NAMESPACE;

#endif // BASEMODULE_H
