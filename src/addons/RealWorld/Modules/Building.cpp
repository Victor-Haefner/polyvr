#include "Building.h"

using namespace OSG;


Building::Building(string id) {
    this->id = id;
}

vector<Vec2f*> Building::getSides() {
    vector<Vec2f*> result;

    for (unsigned int i=0; i<this->positions.size(); i++) {
        Vec2f* side = new Vec2f[2];
        side[0] = this->positions[i];
        side[1] = this->positions[(i+1) % this->positions.size()];
        result.push_back(side);
    }

    return result;
}

vector<Vec2f> Building::getCorners(){
    return positions;
}

bool Building::isClockwise() {
    if (this->positions.size() < 2) return true;

    Vec2f posA = this->positions[0];
    Vec2f posB = this->positions[1];
    Vec2f posAB = posA + ((posB - posA)*0.5f);
    Vec2f dirAB = (posB - posA);
    dirAB.normalize();
    Vec2f dirOrtho = Vec2f(-dirAB.getValues()[1], dirAB.getValues()[0]);

    vector<Vec2f*> sides = this->getSides();
    int intersectionCount = 0;
    for (unsigned int i=1; i<sides.size(); i++) {
        Vec2f* side = sides[i];

        pair<bool, float> intersect = Vec2Helper::lineWithLineSegmentIntersection(posAB, dirOrtho, side[0], side[1]);
        if (intersect.first) {
            intersectionCount++;
        }
    }
//                if (this->id == "101065441") {
//                    printf("%s %d\n", this->id.c_str(), intersectionCount);
//                    printf("ICOUNT=%d, SIDES=%d, POS_AB=%f,%f DIR_ORTHO=%f,%f, side.a=%f,%f side.b=%f,%f\n", intersectionCount, sides.size(),
//                           posAB.getValues()[0], posAB.getValues()[1],
//                           dirOrtho.getValues()[0], dirOrtho.getValues()[1],
//                           side[0].getValues()[0], side[0].getValues()[1],
//                           side[1].getValues()[0], side[1].getValues()[1]
//                           );
//                }

    return ((intersectionCount % 2) == 0);
}

void Building::makeClockwise() {
//            printf("MAKE CLOCKWISE: ");
    if (this->isClockwise()) return;
    reverse(this->positions.begin(), this->positions.end());
    if (!this->isClockwise()) printf("MAKE_CLOCKWISE FAILED!\n");
}

bool Building::isClockwiseCorrect() {
    bool oldCW = this->isClockwise();
    reverse(this->positions.begin(), this->positions.end());
    bool result = (this->isClockwise() != oldCW);
    reverse(this->positions.begin(), this->positions.end());
    return result;
}

bool Building::sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2) {
    return (vai1->pos.getValues()[0] > vai2->pos.getValues()[0]);
}

// checks if points are on same side of a line
bool Building::pointsOnSameSide(Vec2f p1, Vec2f p2, Vec2f l1, Vec2f l2) {
    return ((
            ((p1.getValues()[0] - l1.getValues()[0]) * (l2.getValues()[1] - l1.getValues()[1]) - (l2.getValues()[0] - l1.getValues()[0]) * (p1.getValues()[1] - l1.getValues()[1])) *
            ((p2.getValues()[0] - l1.getValues()[0]) * (l2.getValues()[1] - l1.getValues()[1]) - (l2.getValues()[0] - l1.getValues()[0]) * (p2.getValues()[1] - l1.getValues()[1])))
            > 0);
}

// checks if point is inside of a triangle
bool Building::pointInsideTriangle(Vec2f p, Vec2f a, Vec2f b, Vec2f c) {
    return pointsOnSameSide(p, a, b, c) && pointsOnSameSide(p, b, a, c) && pointsOnSameSide(p, c, a, b);
}

Building::Vec2fWithAdjIdx* Building::copyVai(Vec2fWithAdjIdx* vai) {
    Vec2fWithAdjIdx* res = new Vec2fWithAdjIdx();
    res->pos = vai->pos;
    res->index = vai->index;
    res->adjLeft = vai->adjLeft;
    res->adjRight = vai->adjRight;
    return res;
}

void Building::createTriangles(BuildingStructure* bs, vector<Vec2fWithAdjIdx*>* vertices, Vec2fWithAdjIdx* v) {

    if (v->adjLeft == v->adjRight) return;

    Vec2f a = v->pos;
    Vec2f b = v->adjLeft->pos;
    Vec2f c = v->adjRight->pos;

    for (unsigned int i = vertices->size()-1; i >= 0; i--) {
        Vec2fWithAdjIdx* vp = vertices->at(i);
        if (vp == v->adjLeft || vp == v->adjRight) continue;

        Vec2f p = vp->pos;

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

BuildingStructure* Building::getStructure() {
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
    sort(vertices.begin(), vertices.end(), Building::sortVerticesX);
    reverse(vertices.begin(), vertices.end());

    // do actual triangulation
    while (vertices.size() > 0) {
        Vec2fWithAdjIdx* v = vertices.back();
        vertices.pop_back();
        createTriangles(bs, &vertices, v);
    }

    return bs;
}
