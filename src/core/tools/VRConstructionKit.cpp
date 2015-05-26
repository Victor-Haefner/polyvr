#include "VRConstructionKit.h"
#include "VRSnappingEngine.h"
#include "VRSelector.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"

OSG_BEGIN_NAMESPACE;

VRConstructionKit::VRConstructionKit() {
    snapping = new VRSnappingEngine();
    selector = new VRSelector();
}

VRSnappingEngine* VRConstructionKit::getSnappingEngine() { return snapping; }
VRSelector* VRConstructionKit::getSelector() { return selector; }

int VRConstructionKit::addAnchor(float size, Vec3f color) {
    auto g = new VRGeometry("anchor");
    string bs = toString(size);
    g->setPrimitive("Box", bs + " " + bs + " " + bs + " 1 1 1");
}

OSG_END_NAMESPACE;
