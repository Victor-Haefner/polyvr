#include "Tsdf.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const TSDF& t) { return "TSDF"; }

TSDF::TSDF(Vec3i s) : size(s) {
    size_t N = s[0]*s[1]*s[2];
    field = vector<float>(N);
}

TSDF::~TSDF() {}
TSDFPtr TSDF::create(Vec3i size) { return TSDFPtr( new TSDF(size) ); }
bool TSDF::inside(Vec3i& p) { return (p[0]>=0 && p[1]>=0 && p[2]>=0 && p[0]<size[0] && p[1]<size[1] && p[2]<size[2]); }
size_t TSDF::index(Vec3i& p) { return p[0] + p[1]*size[0] + p[2]*size[0]*size[1]; }
void TSDF::set(float f, Vec3i p) { if (inside(p)) field[index(p)] = f; }
float TSDF::get(Vec3i p) { return inside(p) ? field[index(p)] : 1e6; }
