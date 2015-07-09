#ifndef VRCONSTRUCTIONKIT_H_INCLUDED
#define VRCONSTRUCTIONKIT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSnappingEngine;
class VRSelector;
class VRTransform;
class VRGeometry;
class VRObject;

class VRConstructionKit {
    private:
        VRSnappingEngine* snapping = 0;
        VRSelector* selector = 0;
        VRObject* root = 0;

        map<int, VRGeometry*> anchors;
        map<VRTransform*, VRTransform*> objects;

        int ID();

    public:
        VRConstructionKit();

        VRSnappingEngine* getSnappingEngine();
        VRSelector* getSelector();

        int addAnchorType(float size, Vec3f color);
        void addObject(VRTransform* t);
        VRGeometry* addObjectAnchor(VRTransform* t, int a, Vec3f pos, float radius);

        void breakup(VRTransform* obj);
};

OSG_END_NAMESPACE;

#endif // VRCONSTRUCTIONKIT_H_INCLUDED
