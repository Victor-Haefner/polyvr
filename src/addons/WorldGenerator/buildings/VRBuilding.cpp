#include "VRBuilding.h"
#include "../terrain/VRTerrain.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/math/triangulator.h"

using namespace OSG;

VRBuilding::VRBuilding() {
    wallType = rand();
    windowType = rand();
    doorType = rand();
    roofType = rand();
}

VRBuilding::~VRBuilding() {}

VRBuildingPtr VRBuilding::create() { return VRBuildingPtr( new VRBuilding() ); }

void VRBuilding::addFoundation(VRPolygon polygon, float H) {
    auto t = terrain.lock();
    if (t && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        t->elevatePoint(median);
        ground = median[1];
    }
    foundations.push_back( make_pair(H, polygon) );
}

void VRBuilding::addFloor(VRPolygon polygon, float H) {
    auto t = terrain.lock();
    if (t && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        t->elevatePoint(median);
        ground = median[1];
    }
    stories.push_back( make_pair(H, polygon) );
}

void VRBuilding::addRoof(VRPolygon polygon) { roof = polygon; }

void VRBuilding::computeGeometry(VRGeometryPtr walls, VRGeometryPtr roofs) {
    height = 0;

    // foundations
    for (auto foundation : foundations) {
        float H = foundation.first;
        auto& polygon = foundation.second;

        VRGeoData geo;
        for (int i=0; i<polygon.size(); i++) {
            auto pos1 = polygon.getPoint(i);
            auto pos2 = polygon.getPoint((i+1)%polygon.size());

            float len = (pos2 - pos1).length();
            Vec2d wallDir = (pos2 - pos1);
            wallDir.normalize();

            float wall_segment = 3;

            int segN = floor(len / wall_segment);
            segN = max(segN, 1);
            wall_segment = len / segN;

            float low = ground-H;
            float high = low+H;

            int N = 4;
            float _N = 1./N;

            int fi = N*float(wallType) / RAND_MAX;
            float f_tc1 = fi * _N;
            float f_tc2 = (fi+1) * _N;

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));
                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);
                Color4f c = Color4f(f_tc1, 0, 0, 0.25);
                geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(f_tc1, 0), Vec2d(0, 0.25));
                geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(f_tc2, 0), Vec2d(0, 0.25));
                geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(f_tc2, 0.25), Vec2d(0, 0.5));
                geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(f_tc1, 0.25), Vec2d(0, 0.5));
                geo.pushQuad();
            }
        }
        walls->merge( geo.asGeometry("foundation") );
    }

    // stories
    for (auto story : stories) {
        float H = story.first;
        auto& polygon = story.second;
        VRGeoData geo;
        for (int i=0; i<polygon.size(); i++) {
            auto pos1 = polygon.getPoint(i);
            auto pos2 = polygon.getPoint((i+1)%polygon.size());

            float len = (pos2 - pos1).length();
            Vec2d wallDir = (pos2 - pos1);
            wallDir.normalize();

            float wall_segment = 3;

            int segN = floor(len / wall_segment);
            segN = max(segN, 1);
            wall_segment = len / segN;

            float low = ground+height;
            float high = low+H;

            // insert a door at a random place (when on level 0 && there is enough room)
            int doorIndex = -1;
            if (height == 0 && segN > 2) doorIndex = rand() % segN;

            int N = 4;
            float _N = 1./N;

            int di = N*float(doorType) / RAND_MAX;
            int wi = N*float(windowType) / RAND_MAX;
            int fi = N*float(wallType) / RAND_MAX;

            float d_tc1 = di * _N;
            float d_tc2 = di * _N + _N;
            float w_tc1 = wi * _N;
            float w_tc2 = wi * _N + _N;
            float f_tc1 = fi * _N;
            float f_tc2 = fi * _N + _N;

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));

                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);

                if (i == doorIndex) { // door
                    Color4f c = Color4f(f_tc1, 0, d_tc1, 0.5);
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(f_tc1, 0), Vec2d(d_tc1, 0.5));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(f_tc2, 0), Vec2d(d_tc2, 0.5));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(f_tc2, 0.25), Vec2d(d_tc2, 0.75));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(f_tc1, 0.25), Vec2d(d_tc1, 0.75));
                    geo.pushQuad();
                } else { // window
                    Color4f c = Color4f(f_tc1, 0, w_tc1, 0.25);
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(f_tc1, 0), Vec2d(w_tc1, 0.25));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(f_tc2, 0), Vec2d(w_tc2, 0.25));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(f_tc2, 0.25), Vec2d(w_tc2, 0.5));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(f_tc1, 0.25), Vec2d(w_tc1, 0.5));
                    geo.pushQuad();
                }

            }
        }
        height += H;
        walls->merge( geo.asGeometry("facade") );
    }


    int N = 4;
    float _N = 1./N;
    int ri = N*float(roofType) / RAND_MAX;
    float r_tc1 = ri * _N;

    // roof
    Triangulator t;
    t.add(roof);
    auto g = t.compute();
    g->translate(Vec3d(0,ground+height,0));
    g->applyTransformation();
    g->updateNormals();
    g->setPositionalTexCoords2D(0.05,0,Vec2i(0,2));
    g->setPositionalTexCoords2D(0.05,1,Vec2i(0,2));
    VRGeoData data(g);
    data.addVertexColors(Color4f(r_tc1, 0.75, r_tc1, 0.75));
    roofs->merge(g);
}







