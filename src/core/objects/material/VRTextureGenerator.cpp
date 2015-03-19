#include "VRTextureGenerator.h"
#include "VRPerlin.h"
#include "VRBricks.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTextureGenerator::VRTextureGenerator() {}

void VRTextureGenerator::setSize(Vec3i dim) { width = dim[0]; height = dim[1]; depth = dim[2]; }
void VRTextureGenerator::setSize(int w, int h, int d) { width = w; height = h; depth = d; }

void VRTextureGenerator::add(string type, float amount, Vec3f c1, Vec3f c2) {
    GEN_TYPE t = PERLIN;
    if (type == "Bricks") t = BRICKS;
    add(t, amount, c1, c2);
}

void VRTextureGenerator::add(GEN_TYPE type, float amount, Vec3f c1, Vec3f c2) {
    Layer l;
    l.amount = amount;
    l.type = type;
    l.c1 = c1;
    l.c2 = c2;
    layers.push_back(l);
}

void VRTextureGenerator::clearStage() { layers.clear(); }

ImageRecPtr VRTextureGenerator::compose(int seed) {
    srand(seed);
    Vec3i dims(width, height, depth);

    Vec3f* data = new Vec3f[width*height*depth];
    for (int i=0; i<width*height*depth; i++) data[i] = Vec3f(1,1,1);
    for (auto l : layers) {
        if (l.type == BRICKS) VRBricks::apply(data, dims, l.amount, l.c1, l.c2);
        if (l.type == PERLIN) VRPerlin::apply(data, dims, l.amount, l.c1, l.c2);
    }

    ImageRecPtr img = Image::create();
    img->set(OSG::Image::OSG_RGB_PF, width, height, depth, 0, 1, 0.0, (const uint8_t*)data, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    delete[] data;
    return img;
}

OSG_END_NAMESPACE;
