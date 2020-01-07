#include "VRBuilding.h"
#include "VRDistrict.h"
#include "../terrain/VRTerrain.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/math/triangulator.h"
#include "core/math/path.h"
#include "core/math/pose.h"

using namespace OSG;

VRBuilding::VRBuilding() {
    wallType = rand();
    windowType = rand();
    doorType = rand();
    roofType = rand();
}

VRBuilding::~VRBuilding() {}

VRBuildingPtr VRBuilding::create() { return VRBuildingPtr( new VRBuilding() ); }

VRGeometryPtr VRBuilding::getCollisionShape() {
    auto perimeter = stories[0].second;
    if (perimeter.size() <= 1) return 0;

    auto path = Path::create();
    for (auto p : perimeter.get()) {
        Vec3d p3 = Vec3d(p[0], ground, p[1]);
        path->addPoint( Pose(p3) );
    }
    path->close();
    path->compute(2);

	auto shape = VRStroke::create("shape");
	shape->addPath(path);
	shape->strokeProfile({Vec3d(0, -2, 0), Vec3d(0, 2, 0)}, false, true, false);
	return shape;
}

void VRBuilding::addFoundation(VRPolygon polygon, float H) {
    auto t = terrain.lock();
    if (t && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        median = t->elevatePoint(median);
        ground = median[1];
    }
    foundations.push_back( make_pair(H, polygon) );
}

void VRBuilding::addFloor(VRPolygon polygon, float H) {
    auto t = terrain.lock();
    if (t && ground == 0) {
        Vec3d median = polygon.getBoundingBox().center();
        median = t->elevatePoint(median);
        ground = median[1];
    }
    stories.push_back( make_pair(H, polygon) );
}

void VRBuilding::addRoof(VRPolygon polygon) { roof = polygon; }
void VRBuilding::setType(string t) { type = t; }

void VRBuilding::computeGeometry(VRGeometryPtr walls, VRGeometryPtr roofs, VRDistrictPtr district) {
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
            int fi = N*float(wallType) / RAND_MAX;
            Vec4d UV = district->getChunkUV("walls", fi);

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));
                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);
                Color4f c = Color4f(UV[0], UV[1], 0, 0.25);
                geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(UV[0], UV[1]), Vec2d(0, 0.25));
                geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(UV[2], UV[1]), Vec2d(0, 0.25));
                geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(UV[2], UV[3]), Vec2d(0, 0.5));
                geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(UV[0], UV[3]), Vec2d(0, 0.5));
                geo.pushQuad();
            }
        }
        walls->merge( geo.asGeometry("foundation") );
    }

    // stories
    for (auto story : stories) {
        float H = story.first;
        if (type == "shopping" && height == 0) H += 1;
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
            vector<int> doors;
            if (height == 0) { // compute door layouts
                // special cases
                if (segN == 4) doors.push_back(1+rand()%2);

                // case 1: segN is multiple of 3
                else if (segN >= 3 && segN%3 == 0) {
                    int N = segN/3;
                    for (int i=0; i<N; i++) doors.push_back(1+i*3);
                }

                // case 2: segN is multiple of 5
                else if (segN >= 5 && segN%5 == 0) {
                    int N = segN/5;
                    for (int i=0; i<N; i++) doors.push_back(2+i*5);
                }

                // case 3: segN - 2 is multiple of 3
                else if (segN >= 8 && segN%3 == 2) {
                    int N = (segN-2)/3;
                    for (int i=0; i<N; i++) doors.push_back(2+i*3);
                }

                // case 4: segN - 1 is multiple of 3
                else if (segN >= 7 && segN%3 == 1) {
                    int N = (segN-1)/3;
                    N -= N%2;
                    N /= 2;
                    for (int i=0; i < N; i++) {
                        doors.push_back(2+i*3);
                        doors.push_back(segN-1 - (2+i*3));
                    }
                }
            }

            auto isDoor = [&](int i) {
                for (int j : doors) if (i == j) return true;
                return false;
            };

            int N = 4;
            int di = N*float(doorType) / RAND_MAX;
            int wi = N*float(windowType) / RAND_MAX;
            int fi = N*float(wallType) / RAND_MAX;

            Vec4d dUV = district->getChunkUV("doors", di);
            Vec4d wUV = district->getChunkUV("windows", wi);
            Vec4d fUV = district->getChunkUV("walls", fi);

            for (int i=0; i<segN; i++) {
                Vec2d w1 = pos1 + (wallDir * (i*wall_segment));
                Vec2d w2 = pos1 + (wallDir * ((i+1)*wall_segment));

                Vec2d wallVector = w2-w1;
                Vec3d n = Vec3d(-wallVector[1], 0, wallVector[0]);

                if (type == "shopping" && height == 0) {
                    int si = N*float(rand()) / RAND_MAX;
                    dUV = district->getChunkUV("shops", si);
                    wUV = district->getChunkUV("shops", si);
                }

                if (isDoor(i)) { // door
                    Color4f c = Color4f(fUV[0], fUV[1], dUV[0], dUV[1]);
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(fUV[0], fUV[1]), Vec2d(dUV[0], dUV[1]));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(fUV[2], fUV[1]), Vec2d(dUV[2], dUV[1]));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(fUV[2], fUV[3]), Vec2d(dUV[2], dUV[3]));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(fUV[0], fUV[3]), Vec2d(dUV[0], dUV[3]));
                    geo.pushQuad();
                } else { // window
                    Color4f c = Color4f(fUV[0], fUV[1], wUV[0], wUV[1]);
                    geo.pushVert(Vec3d(w1[0], low, w1[1]), n, c, Vec2d(fUV[0], fUV[1]), Vec2d(wUV[0], wUV[1]));
                    geo.pushVert(Vec3d(w2[0], low, w2[1]), n, c, Vec2d(fUV[2], fUV[1]), Vec2d(wUV[2], wUV[1]));
                    geo.pushVert(Vec3d(w2[0], high, w2[1]), n, c, Vec2d(fUV[2], fUV[3]), Vec2d(wUV[2], wUV[3]));
                    geo.pushVert(Vec3d(w1[0], high, w1[1]), n, c, Vec2d(fUV[0], fUV[3]), Vec2d(wUV[0], wUV[3]));
                    geo.pushQuad();
                }

            }
        }
        height += H;
        walls->merge( geo.asGeometry("facade") );
    }


    int N = 4;
    //float _N = 1./N;
    int ri = N*float(roofType) / RAND_MAX;
    Vec4d rUV = district->getChunkUV("roofs", ri);

    auto roofTop = *roof.shrink(-2);

    float H = 2.0; // roof height

    // roof
    Triangulator t;
    t.add(roofTop);
    auto g = t.compute();
    g->translate(Vec3d(0,ground+height+H,0));
    g->applyTransformation();
    VRGeoData data(g);

    for (int i=0; i<data.size(); i++) data.setNorm(i,Vec3d(0,1,0));

    int Nr = roof.size();
    if (Nr == roofTop.size() && Nr > 0) {
        for (int i=0; i<Nr; i++) {
            Vec2d p1 = roof.getPoint(i);
            Vec2d p2 = roofTop.getPoint(i);
            Vec3d P1 = Vec3d(p1[0], ground+height, p1[1]);
            Vec3d P2 = Vec3d(p2[0], ground+height+H, p2[1]);
            data.pushVert(P1, Vec3d(0,1,0));
            data.pushVert(P2, Vec3d(0,1,0));
        }
        int I = data.size();
        for (int i=0; i<Nr; i++) {
            int j = I-2*Nr+i*2;
            if (i < Nr-1) data.pushQuad(j, j+2, j+3, j+1);
            if (i < Nr-1) data.pushQuad(j, j+2, j+3, j+1);
            else          data.pushQuad(j, I-2*Nr, I-2*Nr+1, j+1);
        }
    }

    data.addVertexColors(Color4f(rUV[0], rUV[1], rUV[0], rUV[1]));
    g->setPositionalTexCoords2D(0.05,0,Vec2i(0,2));
    g->setPositionalTexCoords2D(0.05,1,Vec2i(0,2));
    g->updateNormals(false);
    roofs->merge(g);
}







