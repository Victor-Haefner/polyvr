#include "VRNormalmap.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

VRNormalmap::VRNormalmap() {}

float VRNormalmap::lerp(float a0, float a1, float w) { return (1.0 - w)*a0 + w*a1; }
float VRNormalmap::hermite3(float w) { return 3*pow(w,2) - 2*pow(w,3); }
float VRNormalmap::hermite5(float w) { return 6*pow(w,5) - 15*pow(w,4) + 10*pow(w,3); }

float VRNormalmap::dotGridGradient(Vec3d* grid, Vec3i dim, Vec3i vi, Vec3d v) {
    Vec3d d = v-Vec3d(vi); // Compute the distance vector

    if (vi[0] >= dim[0]) vi[0] = 0; // cyclic boundary condition
    if (vi[1] >= dim[1]) vi[1] = 0;
    if (vi[2] >= dim[2]) vi[2] = 0;

    int k = vi[0]+vi[1]*dim[0]+vi[2]*dim[1]*dim[0];
    return d.dot( grid[k] );
}

float VRNormalmap::perlin(Vec3d* grid, const Vec3i& dim, const Vec3d& v) {
    // Determine grid cell coordinates
    Vec3i v0 = Vec3i(v);
    Vec3d s = v-Vec3d(v0);

    // Interpolate between grid point gradients
    Vec2d n, ix0, ix1, ix2;
    float fu = hermite5(s[0]);
    float fv = hermite5(s[1]);
    float fw = s[2]; //hermite5(s[2]);

    n[0] = dotGridGradient(grid, dim, v0             , v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,0,0), v);
    ix0[0] = lerp(n[0], n[1], fu);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,1,0), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,1,0), v);
    ix0[1] = lerp(n[0], n[1], fu);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,0,1), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,0,1), v);
    ix1[0] = lerp(n[0], n[1], fu);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,1,1), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,1,1), v);
    ix1[1] = lerp(n[0], n[1], fu);

    ix2[0] = lerp(ix0[0], ix0[1], fv);
    ix2[1] = lerp(ix1[0], ix1[1], fv);
    return lerp(ix2[0], ix2[1], fw);
}

vector<Vec2i> computeWindow(Vec3i p, Vec3i d) {
    vector<Vec2i> res;
    for (int i=-1; i<2; i++) {
        for (int j=-1; j<2; j++) {
            int x = p[0]+i;
            int y = p[1]+j;
            if (x < 0) x += d[0];
            if (y < 0) y += d[1];
            if (x >= d[0]) x -= d[0];
            if (y >= d[1]) y -= d[1];
            res.push_back(Vec2i(x,y));
        }
    }
    return res;
}

int computeIndex(Vec3i dims, int x, int y, int z = 0) {
    return z*dims[1]*dims[0] + y*dims[0] + x;
}

template<typename C>
float getHeight(Vec2i p, C* data, Vec3i dims) {
    int i = computeIndex(dims, p[0], p[1]);
    auto c = data[i];
    float h = (c[0]+c[1]+c[2])/3.0;
    return h;
}

void VRNormalmap::apply(Color3f* data, Vec3i dims, float amount) {
    float L = 3*amount;

    size_t N = dims[0]*dims[1]*dims[2];
    Color3f* tmp = new Color3f[N];
    memcpy(tmp, data, N*sizeof(Color3f));

    for (int y=0; y<dims[1]; y++) {
        for (int x=0; x<dims[0]; x++) {
            auto window = computeWindow(Vec3i(x,y,0),dims);
            auto cx1 = getHeight(window[3], tmp, dims);
            auto cx2 = getHeight(window[5], tmp, dims);
            auto cy1 = getHeight(window[1], tmp, dims);
            auto cy2 = getHeight(window[7], tmp, dims);

            float dx = (cx2-cx1);
            float dy = (cy2-cy1);

            Vec3d n = Vec3d(L,0,dx).cross( Vec3d(0,L,dy) );
            n.normalize();
            n = n*0.5+Vec3d(0.5,0.5,0.5);

            int i = computeIndex(dims, x, y);
            data[i][0] = n[0];
            data[i][1] = n[1];
            data[i][2] = n[2];
        }
    }

    delete[] tmp;
}

void VRNormalmap::apply(Color4f* data, Vec3i dims, float amount) {
// TODO
}

OSG_END_NAMESPACE;
