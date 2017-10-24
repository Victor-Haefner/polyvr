#include "VRBridge.h"

using namespace OSG;

VRBridge::VRBridge(VRRoadPtr road) : VRRoadBase("bridge"), road(road) {}
VRBridge::~VRBridge() {}

VRBridgePtr VRBridge::create(VRRoadPtr road) { return VRBridgePtr(new VRBridge(road)); }

VRGeometryPtr VRBridge::createGeometry() {
    return 0;
}
