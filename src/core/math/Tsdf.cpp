#include "Tsdf.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

TSDF::TSDF(Vec3i s) {
    size = new Vec3i(s);
    size_t N = s[0]*s[1]*s[2];
    field = vector<float>(N);
}

TSDF::~TSDF() {
    delete size;
}

TSDFPtr TSDF::create(Vec3i s) { return TSDFPtr( new TSDF(s) ); }
bool TSDF::inside(Vec3i& p) { Vec3i s = *size; return (p[0]>=0 && p[1]>=0 && p[2]>=0 && p[0]<s[0] && p[1]<s[1] && p[2]<s[2]); }
size_t TSDF::index(Vec3i& p) { Vec3i s = *size; return p[0] + p[1]*s[0] + p[2]*s[0]*s[1]; }
void TSDF::set(float f, Vec3i p) { if (inside(p)) field[index(p)] = f; }
float TSDF::get(Vec3i p) { return inside(p) ? field[index(p)] : 1e6; }

VRGeometryPtr TSDF::extractMesh() {
    VRGeoData data;

    auto geo = data.asGeometry("tsdfSurface");
    auto m = VRMaterial::get("tsdfSurfaceMat");
    geo->setMaterial(m);
    return geo;
}
