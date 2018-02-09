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
}

VRBuilding::~VRBuilding() {}

VRBuildingPtr VRBuilding::create() { return VRBuildingPtr( new VRBuilding() ); }

void VRBuilding::addFoundation(VRPolygon polygon, float H) {
    if (terrain && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        terrain->elevatePoint(median);
        ground = median[1];
    }
    foundations.push_back( make_pair(H, polygon) );
}

void VRBuilding::addFloor(VRPolygon polygon, float H) {
    if (terrain && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        terrain->elevatePoint(median);
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
            float e = 0.01;

            int fi = N*float(wallType) / RAND_MAX;
            float f_tc1 = fi * _N + e;
            float f_tc2 = fi * _N - e + _N;

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));
                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);
                geo.pushVert(Vec3d(w1[0], low, w1[1]), n, Vec2d(f_tc1, e), Vec2d(0, 0.25+e));
                geo.pushVert(Vec3d(w2[0], low, w2[1]), n, Vec2d(f_tc2, e), Vec2d(0, 0.25+e));
                geo.pushVert(Vec3d(w2[0], high, w2[1]), n, Vec2d(f_tc2, 0.25-e), Vec2d(0, 0.5-e));
                geo.pushVert(Vec3d(w1[0], high, w1[1]), n, Vec2d(f_tc1, 0.25-e), Vec2d(0, 0.5-e));
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
            float e = 0.01;

            int di = N*float(doorType) / RAND_MAX;
            int wi = N*float(windowType) / RAND_MAX;
            int fi = N*float(wallType) / RAND_MAX;

            float d_tc1 = di * _N + e;
            float d_tc2 = di * _N - e + _N;
            float w_tc1 = wi * _N + e;
            float w_tc2 = wi * _N - e + _N;
            float f_tc1 = fi * _N + e;
            float f_tc2 = fi * _N - e + _N;

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));

                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);

                if (i == doorIndex) { // door
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, Vec2d(f_tc1, e), Vec2d(d_tc1, 0.5+e));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, Vec2d(f_tc2, e), Vec2d(d_tc2, 0.5+e));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, Vec2d(f_tc2, 0.25-e), Vec2d(d_tc2, 0.75-e));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, Vec2d(f_tc1, 0.25-e), Vec2d(d_tc1, 0.75-e));
                    geo.pushQuad();
                } else { // window
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, Vec2d(f_tc1, e), Vec2d(w_tc1, 0.25+e));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, Vec2d(f_tc2, e), Vec2d(w_tc2, 0.25+e));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, Vec2d(f_tc2, 0.25-e), Vec2d(w_tc2, 0.5-e));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, Vec2d(f_tc1, 0.25-e), Vec2d(w_tc1, 0.5-e));
                    geo.pushQuad();
                }

            }
        }
        height += H;
        walls->merge( geo.asGeometry("facade") );
    }

    // roof
    Triangulator t;
    t.add(roof);
    auto g = t.compute();
    g->translate(Vec3d(0,ground+height,0));
    g->applyTransformation();
    g->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
    roofs->merge(g);
}

