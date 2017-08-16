#include "VRBuilding.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/triangulator.h"

using namespace OSG;


VRBuilding::VRBuilding() {}
VRBuilding::~VRBuilding() {}

VRBuildingPtr VRBuilding::create() { return VRBuildingPtr( new VRBuilding() ); }

/*vector<Vec2d*> VRBuilding::getSides() {
    vector<Vec2d*> result;

    for (unsigned int i=0; i<this->positions.size(); i++) {
        Vec2d* side = new Vec2d[2];
        side[0] = this->positions[i];
        side[1] = this->positions[(i+1) % this->positions.size()];
        result.push_back(side);
    }

    return result;
}

vector<Vec2d> VRBuilding::getCorners(){
    return positions;
}

bool VRBuilding::sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2) {
    return (vai1->pos.getValues()[0] > vai2->pos.getValues()[0]);
}

// checks if points are on same side of a line
bool VRBuilding::pointsOnSameSide(Vec2d p1, Vec2d p2, Vec2d l1, Vec2d l2) {
    return ((
            ((p1.getValues()[0] - l1.getValues()[0]) * (l2.getValues()[1] - l1.getValues()[1]) - (l2.getValues()[0] - l1.getValues()[0]) * (p1.getValues()[1] - l1.getValues()[1])) *
            ((p2.getValues()[0] - l1.getValues()[0]) * (l2.getValues()[1] - l1.getValues()[1]) - (l2.getValues()[0] - l1.getValues()[0]) * (p2.getValues()[1] - l1.getValues()[1])))
            > 0);
}

// checks if point is inside of a triangle
bool VRBuilding::pointInsideTriangle(Vec2d p, Vec2d a, Vec2d b, Vec2d c) {
    return pointsOnSameSide(p, a, b, c) && pointsOnSameSide(p, b, a, c) && pointsOnSameSide(p, c, a, b);
}

VRBuilding::Vec2fWithAdjIdx* VRBuilding::copyVai(Vec2fWithAdjIdx* vai) {
    Vec2fWithAdjIdx* res = new Vec2fWithAdjIdx();
    res->pos = vai->pos;
    res->index = vai->index;
    res->adjLeft = vai->adjLeft;
    res->adjRight = vai->adjRight;
    return res;
}

void VRBuilding::createTriangles(BuildingStructure* bs, vector<Vec2fWithAdjIdx*>* vertices, Vec2fWithAdjIdx* v) {

    if (v->adjLeft == v->adjRight) return;

    Vec2d a = v->pos;
    Vec2d b = v->adjLeft->pos;
    Vec2d c = v->adjRight->pos;

    for (unsigned int i = vertices->size()-1; i >= 0; i--) {
        Vec2fWithAdjIdx* vp = vertices->at(i);
        if (vp == v->adjLeft || vp == v->adjRight) continue;

        Vec2d p = vp->pos;

        if (pointInsideTriangle(p, a, b, c)) {
            Vec2fWithAdjIdx* v1 = copyVai(v);
            Vec2fWithAdjIdx* v2 = copyVai(v);
            Vec2fWithAdjIdx* vpExtra = copyVai(vp);

            vp->adjLeft = v1;
            v1->adjRight = vp;

            vpExtra->adjRight = v2;
            v2->adjLeft = vpExtra;

            createTriangles(bs, vertices, v1);
            createTriangles(bs, vertices, v2);
            return;
        }
    }

    // no point P in triangle ABC
    // create triangle ABC
    int* face = new int[3];
    face[0] = v->index;
    face[1] = v->adjLeft->index;
    face[2] = v->adjRight->index;
    bs->faces.push_back(face);

    v->adjLeft->adjRight = v->adjRight;
    v->adjRight->adjLeft = v->adjLeft;
}

BuildingStructure* VRBuilding::getStructure() {
    BuildingStructure* bs = new BuildingStructure();

    vector<Vec2fWithAdjIdx*> vertices;

    // create list of vertices
    for (unsigned int i=0; i < this->positions.size(); i++) {
        bs->vertices.push_back(this->positions[i]);
        Vec2fWithAdjIdx* vai = new Vec2fWithAdjIdx();
        vai->pos = this->positions[i];
        vai->index = i;
        vertices.push_back(vai);
    }

    // save info about adjacent vertices for each vertex
    for (unsigned int i=0; i < vertices.size(); i++) {
        Vec2fWithAdjIdx* v = vertices[i];
        Vec2fWithAdjIdx* vLeft = vertices[(i-1) < 0 ? vertices.size()-1 : i-1];
        Vec2fWithAdjIdx* vRight = vertices[(i+1) % vertices.size()];
        v->adjLeft = vLeft;
        v->adjRight = vRight;
    }

    // sort vertices from right to left
    sort(vertices.begin(), vertices.end(), VRBuilding::sortVerticesX);
    reverse(vertices.begin(), vertices.end());

    // do actual triangulation
    while (vertices.size() > 0) {
        Vec2fWithAdjIdx* v = vertices.back();
        vertices.pop_back();
        createTriangles(bs, &vertices, v);
    }

    return bs;
}*/



VRGeometryPtr VRBuilding::addFloor(VRPolygon polygon, float H) {
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

        float low = height;
        float high = height+H;

        // insert a door at a random place (when on level 0 && there is enough room)
        int doorIndex = -1;
        if (height == 0 && segN > 2) doorIndex = rand() % segN;

        int N = 4;
        float _N = 1./N;
        float e = 0.01;

        int di = N*float(rand()) / RAND_MAX;
        int wi = N*float(rand()) / RAND_MAX;
        int fi = N*float(rand()) / RAND_MAX;

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
    return geo.asGeometry("facade");
}

VRGeometryPtr VRBuilding::addRoof(VRPolygon polygon) {
    Triangulator t;
    t.add(polygon);
    auto g = t.compute();
    g->rotateYonZ();
    g->translate(Vec3f(0,height,0));
    g->applyTransformation();
    return g;
}

