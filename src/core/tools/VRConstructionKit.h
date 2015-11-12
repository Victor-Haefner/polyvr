#ifndef VRCONSTRUCTIONKIT_H_INCLUDED
#define VRCONSTRUCTIONKIT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSnappingEngine;
class VRSelector;

class VRConstructionKit {
    private:
        VRSnappingEngine* snapping = 0;
        VRSelector* selector = 0;
        VRObjectPtr root = 0;

        map<int, VRGeometryPtr> anchors;
        map<VRTransform*, VRTransformPtr> objects;

        int ID();

    public:
        VRConstructionKit();

        void clear();

        VRSnappingEngine* getSnappingEngine();
        VRSelector* getSelector();
        vector<VRObjectPtr> getObjects();

        int addAnchorType(float size, Vec3f color);
        void addObject(VRTransformPtr t);
        VRGeometryPtr addObjectAnchor(VRTransformPtr t, int a, Vec3f pos, float radius);

        void breakup(VRTransformPtr obj);
};

OSG_END_NAMESPACE;

#endif // VRCONSTRUCTIONKIT_H_INCLUDED
