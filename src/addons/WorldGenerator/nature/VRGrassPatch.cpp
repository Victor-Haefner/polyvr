#include "VRGrassPatch.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/math/polygon.h"

using namespace OSG;

VRGrassPatch::VRGrassPatch() : VRTransform("grass_patch") {}
VRGrassPatch::~VRGrassPatch() {}
VRGrassPatchPtr VRGrassPatch::create() { return VRGrassPatchPtr( new VRGrassPatch() ); }

void VRGrassPatch::addGrassBlade(VRGeoData& data) { // Todo: replace by triangle strip
    float w = 0.01; // half blade width
    float h = 0.2; // blade height
    float b = 0.1; // blade bending
    float h1 = h/3;
    float h2 = h*2.0/3;
    float b1 = b/6;
    float b2 = b/2;

    Vec3f n1 = Vec3f(0,b1,-h1);
    Vec3f n2 = Vec3f(0,b2,-h2);
    n1.normalize();
    n2.normalize();

    data.pushVert(Vec3f(-w,0, 0),  Vec3f(0,0,-1));
    data.pushVert(Vec3f( w,0, 0),  Vec3f(0,0,-1));
    data.pushVert(Vec3f(-w,h1,b1), n1);
    data.pushVert(Vec3f( w,h1,b1), n1);
    data.pushVert(Vec3f(-w,h2,b2), n2);
    data.pushVert(Vec3f( w,h2,b2), n2);
    data.pushVert(Vec3f( w,h, b),  Vec3f(0,0,-1));

    data.pushQuad(0,1,3,2);
    data.pushQuad(2,3,5,4);
    data.pushTri(4,5,6);
}

void VRGrassPatch::setArea(PolygonPtr p) {
    area = p;
    Vec3f median = area->getBoundingBox().center();
    setFrom(median);
}

void VRGrassPatch::createLod(VRGeoData& geo, int lvl, Vec3f offset, int ID) {
    Matrix Offset;
    Offset.setTranslate(offset);

    Vec2f id = Vec2f(ID,1); // the 1 is a flag to identify the ID as such!

    if (lods.count(lvl)) {
        geo.append(lods[lvl], Offset);
        return;
    }

    VRGeoData Hull;
    addGrassBlade(Hull);

    lods[lvl] = Hull.asGeometry("grassLod");
    geo.append(Hull, Offset);
}


/**

Idea:

- terrain has texture
- render grass as second pass, using terrain info
- reuse road texture stuff!

*/







