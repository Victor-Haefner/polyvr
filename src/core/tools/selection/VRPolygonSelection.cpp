#include "VRPolygonSelection.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace OSG;

VRPolygonSelection::VRPolygonSelection() {}

void VRPolygonSelection::setOrigin(pose orig) { selection.setPose(orig); }
void VRPolygonSelection::addEdge(Vec3f dir) { selection.addEdge(dir); }
void VRPolygonSelection::close() { selection.close(); }

bool VRPolygonSelection::objSelected(VRGeometryPtr geo) {
    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);

    // get all 8 bb corners
    // test if all corners inside frustum
    return false;
}

bool VRPolygonSelection::partialSelected(VRGeometryPtr geo) { return false; }


bool VRPolygonSelection::vertSelected(Vec3f p) { return false; }
