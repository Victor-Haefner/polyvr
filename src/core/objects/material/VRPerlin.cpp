#include "VRPerlin.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRPerlin::VRPerlin() {}

float VRPerlin::lerp(float a0, float a1, float w) { return (1.0 - w)*a0 + w*a1; }

float VRPerlin::dotGridGradient(Vec3f* grid, Vec3i dim, Vec3i vi, Vec3f v) {
    Vec3f d = v-Vec3f(vi); // Compute the distance vector

    if (vi[0] >= dim[0]) vi[0] = 0; // cyclic
    if (vi[1] >= dim[1]) vi[1] = 0;
    if (vi[2] >= dim[2]) vi[2] = 0;

    Vec3f g = grid[vi[0]+vi[1]*dim[0]+vi[2]*dim[1]*dim[0]]; // Compute the dot-product
    return d.dot(g);
}

float VRPerlin::perlin(Vec3f* grid, const Vec3i& dim, const Vec3f& v) {
    // Determine grid cell coordinates
    Vec3i v0 = Vec3i(v);
    Vec3f s = v-Vec3f(v0);

    // Interpolate between grid point gradients
    Vec2f n, ix0, ix1, ix2;

    n[0] = dotGridGradient(grid, dim, v0, v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,0,0), v);
    ix0[0] = lerp(n[0], n[1], s[0]);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,1,0), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,1,0), v);
    ix0[1] = lerp(n[0], n[1], s[0]);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,0,1), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,0,1), v);
    ix1[0] = lerp(n[0], n[1], s[0]);

    n[0] = dotGridGradient(grid, dim, v0+Vec3i(0,1,1), v);
    n[1] = dotGridGradient(grid, dim, v0+Vec3i(1,1,1), v);
    ix1[1] = lerp(n[0], n[1], s[0]);

    ix2[0] = lerp(ix0[0], ix0[1], s[1]);
    ix2[1] = lerp(ix1[0], ix1[1], s[1]);
    return lerp(ix2[0], ix2[1], s[2]);
}

void VRPerlin::apply(Vec3f* data, Vec3i dims, float amount, Vec3f c1, Vec3f c2) {
    float a = 0.5*amount;
    Vec3i dim(dims[0]*a, dims[1]*a, dims[2]*a);
    for (int i=0; i<3; i++) dim[i] = max(1,dim[i]);
    Vec3f* grid = new Vec3f[ dim[0]*dim[1]*dim[2] ];
    VRPerlin perl;

    // noise grid
    for (int k=0; k<dim[2]; k++) {
        for (int j=0; j<dim[1]; j++) {
            for (int i=0; i<dim[0]; i++) {
                Vec3f r = Vec3f(rand(), rand(), rand())*2.0/RAND_MAX - Vec3f(1,1,1);
                r.normalize();
                grid[i+j*dim[0]+k*dim[1]*dim[0]] = r;
            }
        }
    }

    // compute noise in grid and mix with data
    for (int z=0; z<dims[2]; z++) {
        for (int y=0; y<dims[1]; y++) {
            for (int x=0; x<dims[0]; x++) {
                float p = perl.perlin(grid, dim, Vec3f(x,y,z)*a );
                Vec3f c = p*c1 + (1-p)*c2;

                int i = z*dims[1]*dims[0] + y*dims[0] + x;
                data[i][0] *= c[0];
                data[i][1] *= c[1];
                data[i][2] *= c[2];
            }
        }
    }

    delete[] grid;
}

void VRPerlin::apply(Vec4f* data, Vec3i dims, float amount, Vec4f c1, Vec4f c2) {
    float a = 0.5*amount;
    Vec3i dim(dims[0]*a, dims[1]*a, dims[2]*a);
    for (int i=0; i<3; i++) dim[i] = max(1,dim[i]);
    Vec3f* grid = new Vec3f[ dim[0]*dim[1]*dim[2] ];
    VRPerlin perl;

    // noise grid
    for (int k=0; k<dim[2]; k++) {
        for (int j=0; j<dim[1]; j++) {
            for (int i=0; i<dim[0]; i++) {
                Vec3f r = Vec3f(rand(), rand(), rand())*2.0/RAND_MAX - Vec3f(1,1,1);
                r.normalize();
                grid[i+j*dim[0]+k*dim[1]*dim[0]] = r;
            }
        }
    }

    // compute noise in grid and mix with data
    for (int z=0; z<dims[2]; z++) {
        for (int y=0; y<dims[1]; y++) {
            for (int x=0; x<dims[0]; x++) {
                float p = perl.perlin(grid, dim, Vec3f(x,y,z)*a );
                Vec4f c = p*c1 + (1-p)*c2;

                int i = z*dims[1]*dims[0] + y*dims[0] + x;
                data[i][0] *= c[0];
                data[i][1] *= c[1];
                data[i][2] *= c[2];
                data[i][3] *= c[3];
            }
        }
    }

    delete[] grid;
}

OSG_END_NAMESPACE;
