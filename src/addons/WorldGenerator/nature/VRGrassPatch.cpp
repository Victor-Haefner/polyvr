#include "VRGrassPatch.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/math/polygon.h"

const double pi = 2*acos(0.0);

using namespace OSG;

VRGrassPatch::VRGrassPatch() : VRTransform("grass_patch") {}

VRGrassPatch::~VRGrassPatch() {}
VRGrassPatchPtr VRGrassPatch::create() { return VRGrassPatchPtr( new VRGrassPatch() ); }

void VRGrassPatch::setArea(PolygonPtr p) {
    area = p;
    VRGeoData patch;
    createPatch(patch);
    addChild( patch.asGeometry("grassPatch") );
}

void VRGrassPatch::addGrassBlade(VRGeoData& data, Vec3f pos, float a, float dh, Vec3f c) { // Todo: replace by triangle strip
    float h = bladeHeight*dh;
    float w = bladeHeight*0.05; // 0.01 half blade width
    float b = bladeHeight*0.33; // blade bending

    float ca = cos(a);
    float sa = sin(a);

    auto rot = [&](const Vec3f& v) {
        float X = v[0]*ca - v[2]*sa;
        float Y = v[0]*sa + v[2]*ca;
        return Vec3f(X, v[1], Y);
    };

    float h1 = h/3;
    float h2 = h*2.0/3;
    float b1 = b/6;
    float b2 = b/2;

    Vec3f p1 = rot(Vec3f( w,0, 0));
    Vec3f p2 = rot(Vec3f(-w,0, 0));
    Vec3f p3 = rot(Vec3f( w,h1,b1));
    Vec3f p4 = rot(Vec3f(-w,h1,b1));
    Vec3f p5 = rot(Vec3f( w,h2,b2));
    Vec3f p6 = rot(Vec3f(-w,h2,b2));
    Vec3f p7 = rot(Vec3f( 0,h, b));

    Vec3f n0 = rot(Vec3f(0,0,-1));
    Vec3f n1 = rot(Vec3f(0,b1,-h1));
    Vec3f n2 = rot(Vec3f(0,b2,-h2));
    n1.normalize();
    n2.normalize();

    int N = data.size();

    data.pushVert(pos + p1, n0, c);
    data.pushVert(pos + p2, n0, c);
    data.pushVert(pos + p3, n1, c);
    data.pushVert(pos + p4, n1, c);
    data.pushVert(pos + p5, n2, c);
    data.pushVert(pos + p6, n2, c);
    data.pushVert(pos + p7, n0, c);

    data.pushQuad(N,N+1,N+3,N+2);
    data.pushQuad(N+2,N+3,N+5,N+4);
    data.pushTri(N+4,N+5,N+6);
}

void VRGrassPatch::createPatch(VRGeoData& data, int lvl) {
    int N = 100*area->computeArea();
    for (int i=0; i<N; i++) {
        Vec3f pos = area->getRandomPoint();
        addGrassBlade(data, pos, 2*pi*float(rand())/RAND_MAX, 0.5 + 0.5*float(rand())/RAND_MAX, Vec3f(0.2,0.8,0.0));
    }
}

void VRGrassPatch::createLod(VRGeoData& geo, int lvl, Vec3f offset, int ID) {
    Matrix Offset;
    Offset.setTranslate(offset);

    Vec2f id = Vec2f(ID, 1); // the 1 is a flag to identify the ID as such!

    if (lods.count(lvl)) {
        geo.append(lods[lvl], Offset);
        return;
    }

    VRGeoData Hull;
    createPatch(Hull);

    lods[lvl] = Hull.asGeometry("grassLod");
    geo.append(Hull, Offset);
}


/**

Idea:

- terrain has texture
- render grass as second pass, using terrain info
- reuse road texture stuff!

*/







