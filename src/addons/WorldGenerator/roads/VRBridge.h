#ifndef VRBRIDGE_H_INCLUDED
#define VRBRIDGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRRoadBase.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBridge : public VRRoadBase {
    private:
        VRRoadPtr road;

    public:
        VRBridge(VRRoadPtr road);
        ~VRBridge();

        static VRBridgePtr create(VRRoadPtr road);

        VRGeometryPtr createGeometry();
};

OSG_END_NAMESPACE;

#endif // VRBRIDGE_H_INCLUDED
