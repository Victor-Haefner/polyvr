#ifndef VRCONSTRUCTIONKIT_H_INCLUDED
#define VRCONSTRUCTIONKIT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSnappingEngine;
class VRSelector;
class VRGeometry;
class VRObject;

class VRConstructionKit {
    private:
        VRSnappingEngine* snapping = 0;
        VRSelector* selector = 0;

        map<int, VRGeometry*> anchors;

        VRObject* root = 0;

    public:
        VRConstructionKit();

        VRSnappingEngine* getSnappingEngine();
        VRSelector* getSelector();

        int addAnchor(float size, Vec3f color);
};

OSG_END_NAMESPACE;

#endif // VRCONSTRUCTIONKIT_H_INCLUDED
