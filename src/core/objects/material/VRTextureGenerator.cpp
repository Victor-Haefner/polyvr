#include "VRTextureGenerator.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

float lerp(float a0, float a1, float w) { return (1.0 - w)*a0 + w*a1; }

float dotGridGradient(Vec2f* grid, Vec2i dim, Vec2i vi, Vec2f v) {
    Vec2f d = v-Vec2f(vi); // Compute the distance vector

    if (vi[0] >= dim[0]) vi[0] = 0; // cyclic
    if (vi[1] >= dim[1]) vi[1] = 0;

    Vec2f g = grid[vi[0]+vi[1]*dim[0]]; // Compute the dot-product
    return d.dot(g);
}

float perlin(Vec2f* grid, Vec2i dim, Vec2f v) {
    // Determine grid cell coordinates
    Vec2i v0 = Vec2i(v);
    Vec2f s = v-Vec2f(v0);

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(grid, dim, v0, v);
    n1 = dotGridGradient(grid, dim, v0+Vec2i(1,0), v);
    ix0 = lerp(n0, n1, s[0]);

    n0 = dotGridGradient(grid, dim, v0+Vec2i(0,1), v);
    n1 = dotGridGradient(grid, dim, v0+Vec2i(1,1), v);
    ix1 = lerp(n0, n1, s[0]);

    value = lerp(ix0, ix1, s[1]);
    return value;
}

VRTextureGenerator::VRTextureGenerator() {}

void VRTextureGenerator::setSize(int w, int h) { width = w; height = h; }

void VRTextureGenerator::addNoise(NOISE_TYPE noise, float amount, Vec3f c1, Vec3f c2) {
    noise_layer l;
    l.amount = amount;
    l.noise = noise;
    l.c1 = c1;
    l.c2 = c2;
    layers.push_back(l);
}

void VRTextureGenerator::clearStage() { layers.clear(); }

ImageRecPtr VRTextureGenerator::compose(int seed) {
    srand(seed);

    Vec3f* data = new Vec3f[width*height];
    for (auto l : layers) {
        Vec2i dim = Vec2i(width*0.5*l.amount, height*0.5*l.amount);
        Vec2f* grid = new Vec2f[ dim[0]*dim[1] ];

        for (int i=0; i<dim[0]; i++) {
            for (int j=0; j<dim[1]; j++) {
                Vec2f r = Vec2f(rand(), rand())*2.0/RAND_MAX - Vec2f(1,1);
                r.normalize();
                grid[i+j*dim[0]] = r;
            }
        }

        for (int x=0; x<width; x++) {
            for (int y=0; y<height; y++) {
                float p = perlin(grid, dim, Vec2f(x,y)*l.amount*0.5 );
                data[y*height+x] =  p*l.c1 + (1-p)*l.c2;
            }
        }

        delete grid;
    }

    ImageRecPtr img = Image::create();
    img->set(OSG::Image::OSG_RGB_PF, width, height, 1, 0, 1, 0.0, (const uint8_t*)data, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    return img;
}

OSG_END_NAMESPACE;
