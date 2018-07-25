#include "Asset.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace OSG;

map<string, VRGeometryPtr> Asset::assets = map<string, VRGeometryPtr>();

VRGeometryPtr Asset::merge(VRObjectPtr obj) {
    auto geos = obj->getChildren(1, "Geometry");
    if (geos.size() == 0) return VRGeometry::create(obj->getBaseName());
    auto trans = VRTransform::getParentTransform(obj);

    VRGeoData geo;
    for (auto o : geos) {
        auto g = dynamic_pointer_cast<VRGeometry>(o);
        Matrix4d m = trans->getMatrixTo(g);
        geo.append(g, m);
    }

    return geo.asGeometry(obj->getName());
}
