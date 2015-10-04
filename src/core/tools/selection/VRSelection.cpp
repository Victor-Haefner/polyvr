#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeometry.h>

using namespace OSG;

bool VRSelection::vertSelected(Vec3f p) { return false; }
bool VRSelection::objSelected(VRGeometryPtr geo) { return false; }
bool VRSelection::partialSelected(VRGeometryPtr geo) { return false; }

VRSelection::VRSelection() {}

void VRSelection::apply(VRObjectPtr tree) {
    auto geos = tree->getChildren(true, "Geometry");
    for (auto g : geos) {
        VRGeometryPtr geo = static_pointer_cast<VRGeometry>(g);
        if ( objSelected(geo) ) selected.push_back(geo);
        else if ( partialSelected(geo) ) partials.push_back(geo);
        else continue;
    }
}

vector<VRGeometryWeakPtr> VRSelection::getPartials() { return partials; }
vector<VRGeometryWeakPtr> VRSelection::getSelected() { return selected; }

vector<int> VRSelection::getSelectedVertices(VRGeometryPtr geo) {
    vector<int> res;
    auto pos = geo->getMesh()->getPositions();
    for (int i=0; i<pos->size(); i++) {
        Vec3f p = pos->getValue<Vec3f>(i);
        if (vertSelected(p)) res.push_back(i);
    }
    return res;
}
