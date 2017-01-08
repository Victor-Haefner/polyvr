#include "VRTextureGenerator.h"
#include "VRPerlin.h"
#include "VRBricks.h"
#include "core/networking/VRSharedMemory.h"

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGMatrixUtility.h>
#include "core/objects/material/VRTexture.h"
#include "core/math/path.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTextureGenerator::VRTextureGenerator() {}
VRTextureGenerator::~VRTextureGenerator() {}
shared_ptr<VRTextureGenerator> VRTextureGenerator::create() { return shared_ptr<VRTextureGenerator>(new VRTextureGenerator()); }

void VRTextureGenerator::setSize(Vec3i dim, bool a) { width = dim[0]; height = dim[1]; depth = dim[2]; hasAlpha = a; }
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
    l.c31 = c1;
    l.c32 = c2;
    l.Nchannels = 3;
    layers.push_back(l);
}

void VRTextureGenerator::add(string type, float amount, Vec4f c1, Vec4f c2) {
    GEN_TYPE t = PERLIN;
    if (type == "Bricks") t = BRICKS;
    add(t, amount, c1, c2);
}

void VRTextureGenerator::add(GEN_TYPE type, float amount, Vec4f c1, Vec4f c2) {
    Layer l;
    l.amount = amount;
    l.type = type;
    l.c41 = c1;
    l.c42 = c2;
    l.Nchannels = 4;
    layers.push_back(l);
}

void VRTextureGenerator::drawFill(Vec4f c) {
    Layer l;
    l.type = FILL;
    l.c41 = c;
    layers.push_back(l);
}

void VRTextureGenerator::drawPixel(Vec3i p, Vec4f c) {
    Layer l;
    l.type = PIXEL;
    l.p1 = p;
    l.c41 = c;
    layers.push_back(l);
}

void VRTextureGenerator::drawLine(Vec3f p1, Vec3f p2, Vec4f c, float w) {
    Layer l;
    l.type = LINE;
    l.c31 = p1;
    l.c32 = p2;
    l.c41 = c;
    l.amount = w;
    layers.push_back(l);
}

void VRTextureGenerator::drawPath(pathPtr p, Vec4f c, float w) {
    Layer l;
    l.type = PATH;
    l.p = p;
    l.c41 = c;
    l.amount = w;
    layers.push_back(l);
}

void VRTextureGenerator::applyFill(Vec3f* data, Vec4f c) {
    for (int k=0; k<depth; k++) {
        for (int j=0; j<height; j++) {
            for (int i=0; i<width; i++) {
                int d = k*height*width + j*width + i;
                data[d] = Vec3f(c[0], c[1], c[2])*c[3];
            }
        }
    }
}

void VRTextureGenerator::applyFill(Vec4f* data, Vec4f c) {
    for (int k=0; k<depth; k++) {
        for (int j=0; j<height; j++) {
            for (int i=0; i<width; i++) {
                int d = k*height*width + j*width + i;
                data[d] = c;
            }
        }
    }
}

bool VRTextureGenerator::inBox(Pnt3f& p, Vec3f& s) {
    if (abs(p[0]) > s[0]) return false;
    if (abs(p[1]) > s[1]) return false;
    if (abs(p[2]) > s[2]) return false;
    return true;
}

void VRTextureGenerator::applyPixel(Vec4f* data, Vec3i p, Vec4f c) {
    int d = p[2]*height*width + p[1]*width + p[0];
    int N = depth*height*width;
    if (d >= N || d < 0) { cout << "Warning: applyPixel failed, pixel " << d << " " << p << " " << height << " " << width << " " << depth << " out of range! (buffer size is " << N << ")" << endl; return; }
    data[d] = Vec4f(c[0], c[1], c[2], 1.0)*c[3] + data[d]*(1.0-c[3]);
}

void VRTextureGenerator::applyPixel(Vec3f* data, Vec3i p, Vec4f c) {
    int d = p[2]*height*width + p[1]*width + p[0];
    int N = depth*height*width;
    if (d >= N|| d < 0) { cout << "Warning: applyPixel failed, pixel " << d << " " << p << " " << height << " " << width << " " << depth << " out of range! (buffer size is " << N << ")" << endl; return; }
    data[d] = Vec3f(c[0], c[1], c[2])*c[3] + data[d]*(1.0-c[3]);
}

void VRTextureGenerator::applyLine(Vec3f* data, Vec3f p1, Vec3f p2, Vec4f c, float w) {
    Vec3f pm = (p1+p2)*0.5;
    Vec3f d = p2-p1;
    Vec3f u = Vec3f(0,1,0);
    if (d[1] > d[2]) u = Vec3f(0,0,-1);

    float l2 = (p2-p1).length()*0.5;
    float w2 = w*0.5;

    Matrix M; // box transformation
    MatrixLookAt(M, pm, p1, u);

    int X2 = ceil(w2*width);
    int Y2 = ceil(w2*height);
    int L2 = ceil(l2*depth);
    int N = depth*height*width;

    for (int k=-L2; k<=L2; k++) {
        for (int j=-Y2; j<=Y2; j++) {
            for (int i=-X2; i<=X2; i++) {
                Pnt3f pi = Pnt3f(float(i)/width, float(j)/height, float(k)/depth);
                M.mult(pi, pi); // in tex coords
                Vec3i pI(pi[0]*width, pi[1]*height, pi[2]*depth);
                applyPixel(data, pI+Vec3i(0,0,0), c);
                applyPixel(data, pI+Vec3i(1,0,0), c);
                applyPixel(data, pI+Vec3i(0,1,0), c);
                applyPixel(data, pI+Vec3i(0,0,1), c);
                applyPixel(data, pI+Vec3i(1,1,0), c);
                applyPixel(data, pI+Vec3i(0,1,1), c);
                applyPixel(data, pI+Vec3i(1,0,1), c);
                applyPixel(data, pI+Vec3i(1,1,1), c);
            }
        }
    }
}

void VRTextureGenerator::applyLine(Vec4f* data, Vec3f p1, Vec3f p2, Vec4f c, float w) {
    Vec3f pm = (p1+p2)*0.5;
    Vec3f d = p2-p1;
    Vec3f u = Vec3f(0,1,0);
    if (d[1] > d[2]) u = Vec3f(0,0,-1);

    float l2 = d.length()*0.5;
    float w2 = w*0.5;

    Matrix M; // box transformation
    MatrixLookAt(M, pm, p1, u);

    int X2 = ceil(w2*width);
    int Y2 = ceil(w2*height);
    int L2 = ceil(l2*depth);
    int N = depth*height*width;

    for (int k=-L2; k<=L2; k++) {
        for (int j=-Y2; j<=Y2; j++) {
            for (int i=-X2; i<=X2; i++) {
                Pnt3f pi = Pnt3f(float(i)/width, float(j)/height, float(k)/depth);
                M.mult(pi, pi); // in tex coords
                Vec3i pI(pi[0]*width, pi[1]*height, pi[2]*depth);
                applyPixel(data, pI+Vec3i(0,0,0), c);
                applyPixel(data, pI+Vec3i(1,0,0), c);
                applyPixel(data, pI+Vec3i(0,1,0), c);
                applyPixel(data, pI+Vec3i(0,0,1), c);
                applyPixel(data, pI+Vec3i(1,1,0), c);
                applyPixel(data, pI+Vec3i(0,1,1), c);
                applyPixel(data, pI+Vec3i(1,0,1), c);
                applyPixel(data, pI+Vec3i(1,1,1), c);
            }
        }
    }
}

// TODO: fix holes between curved path segments
void VRTextureGenerator::applyPath(Vec3f* data, pathPtr p, Vec4f c, float w) {
    auto pos = p->getPositions();
    for (uint i=1; i<pos.size(); i++) {
        applyLine(data, pos[i-1], pos[i], c, w);
    }
}

void VRTextureGenerator::applyPath(Vec4f* data, pathPtr p, Vec4f c, float w) {
    auto pos = p->getPositions();
    for (uint i=1; i<pos.size(); i++) {
        applyLine(data, pos[i-1], pos[i], c, w);
    }
}

void VRTextureGenerator::clearStage() { layers.clear(); }

VRTexturePtr VRTextureGenerator::compose(int seed) {
    srand(seed);
    Vec3i dims(width, height, depth);

    Vec3f* data3 = new Vec3f[width*height*depth];
    Vec4f* data4 = new Vec4f[width*height*depth];
    for (int i=0; i<width*height*depth; i++) data3[i] = Vec3f(1,1,1);
    for (int i=0; i<width*height*depth; i++) data4[i] = Vec4f(1,1,1,1);
    for (auto l : layers) {
        if (!hasAlpha) {
            if (l.type == BRICKS) VRBricks::apply(data3, dims, l.amount, l.c31, l.c32);
            if (l.type == PERLIN) VRPerlin::apply(data3, dims, l.amount, l.c31, l.c32);
            if (l.type == LINE) applyLine(data3, l.c31, l.c32, l.c41, l.amount);
            if (l.type == FILL) applyFill(data3, l.c41);
            if (l.type == PIXEL) applyPixel(data3, l.p1, l.c41);
            if (l.type == PATH) applyPath(data3, l.p, l.c41, l.amount);
        }
        if (hasAlpha) {
            if (l.type == BRICKS) VRBricks::apply(data4, dims, l.amount, l.c41, l.c42);
            if (l.type == PERLIN) VRPerlin::apply(data4, dims, l.amount, l.c41, l.c42);
            if (l.type == LINE) applyLine(data4, l.c31, l.c32, l.c41, l.amount);
            if (l.type == FILL) applyFill(data4, l.c41);
            if (l.type == PIXEL) applyPixel(data4, l.p1, l.c41);
            if (l.type == PATH) applyPath(data4, l.p, l.c41, l.amount);
        }
    }

    img = VRTexture::create();
    auto format = hasAlpha ? OSG::Image::OSG_RGBA_PF : OSG::Image::OSG_RGB_PF;
    if (hasAlpha) img->getImage()->set(format, width, height, depth, 0, 1, 0.0, (const uint8_t*)data4, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    else       img->getImage()->set(format, width, height, depth, 0, 1, 0.0, (const uint8_t*)data3, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);
    delete[] data3;
    delete[] data4;
    return img;
}

struct tex_params {
    Vec3i dims;
    int pixel_format = OSG::Image::OSG_RGB_PF;
    int data_type = OSG::Image::OSG_FLOAT32_IMAGEDATA;
    int internal_pixel_format = -1;
};

VRTexturePtr VRTextureGenerator::readSharedMemory(string segment, string object) {
    VRSharedMemory sm(segment, false);

    // add texture example
    /*auto s = sm.addObject<Vec3i>(object+"_size");
    *s = Vec3i(2,2,1);
    auto vec = sm.addVector<Vec3f>(object);
    vec->push_back(Vec3f(1,0,1));
    vec->push_back(Vec3f(1,1,0));
    vec->push_back(Vec3f(0,0,1));
    vec->push_back(Vec3f(1,0,0));*/

    // read texture
    auto tparams = sm.getObject<tex_params>(object+"_size");
    auto vs = tparams.dims;
    auto vdata = sm.getVector<float>(object);

    //cout << "read shared texture " << object << " " << vs << "   " << tparams.pixel_format << "  " << tparams.data_type << "  " << vdata.size() << endl;

    img = VRTexture::create();
    img->getImage()->set(tparams.pixel_format, vs[0], vs[1], vs[2], 0, 1, 0.0, (const uint8_t*)&vdata[0], tparams.data_type, true, 1);
    img->setInternalFormat( tparams.internal_pixel_format );
    return img;
}

OSG_END_NAMESPACE;
