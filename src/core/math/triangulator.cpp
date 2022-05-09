#include "triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include <GL/glut.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

void Triangulator::testQuad() {
    VRPolygon p1;
    p1.addPoint(Vec2d(-2,3));
    p1.addPoint(Vec2d(-2,0));
    p1.addPoint(Vec2d(2,0));
    p1.addPoint(Vec2d(2,3));

    VRPolygon p2;
    p2.addPoint(Vec2d(-1,2));
    p2.addPoint(Vec2d(-1,1));
    p2.addPoint(Vec2d(1,1));
    p2.addPoint(Vec2d(1,2));

    add(p1);
    add(p2, false);
}

Triangulator::Triangulator() {}
Triangulator::~Triangulator() {}
shared_ptr<Triangulator> Triangulator::create() { return shared_ptr<Triangulator>(new Triangulator()); }

bool samePnt2(Vec2d a, Vec2d b) { return ((b-a).length() < 1e-3); }
bool samePnt3(Vec3d a, Vec3d b) { return ((b-a).length() < 1e-3); }

void Triangulator::add(VRPolygon poly, bool outer) {
    if (poly.size3() > 1) {
        int i = poly.size3()-1;
        if (samePnt3(poly.getPoint3(0), poly.getPoint3(i))) poly.remPoint3(i);
    }

    if (poly.size2() > 1) {
        int i = poly.size2()-1;
        if (samePnt2(poly.getPoint(0), poly.getPoint(i))) poly.remPoint(i);
    }

    if (outer) outer_bounds.push_back(poly);
    else inner_bounds.push_back(poly);
}

VRGeometryPtr Triangulator::computeBounds() {
    VRGeoData geo;

    auto drawPoly = [&](VRPolygon& poly, Color3f col) {
        vector<Vec3d> points;
        if (poly.size2() > 2) points = toSpace(poly.get());
        if (poly.size3() > 2) points = poly.get3();
        for (int i=0; i<points.size(); i++) {
            auto& p = points[i];
            geo.pushVert(Pnt3d(p), Vec3d(0,1,0), col);
            if (i > 0) geo.pushLine();
        }
    };

    for (auto& poly : outer_bounds) drawPoly(poly, Color3f(0,1,0));
    for (auto& poly : inner_bounds) drawPoly(poly, Color3f(1,0,0));
    return geo.asGeometry("tessBounds");
}

VRGeometryPtr Triangulator::compute() {
    tessellate();
    return geo ? geo->asGeometry("tessellation") : 0;
}

void Triangulator::append(VRGeoDataPtr data, bool aN) {
    addNormals = aN;
    geo = data;
    tessellate();
    geo = 0;
}

// GLU_TESS CALLBACKS
void tessBeginCB(GLenum which, void* data) {
    auto Self = (Triangulator*)data;
    if (!Self->geo) Self->geo = VRGeoData::create();
    Self->geo->pushType(which);
    Self->num_points = 0;
    //cout << "beg " << which << endl;
}

void tessEndCB(void* data) {
    auto Self = (Triangulator*)data;
    int Nprim = Self->num_points;

    int Ni0 = Self->geo->getNIndices();
    for (int i=0; i<Nprim; i++) Self->geo->pushIndex( Ni0 + i );

    Self->geo->pushLength(Nprim);
    //Self->geo->updateType(Self->current_primitive, Self->num_points);
    //cout << "end" << endl;
}

void tessVertexCB(const GLvoid* vertices, void* data) { // draw a vertex
    const GLdouble *ptr = (const GLdouble*)vertices;
    Pnt3d p(*ptr, *(ptr+1), *(ptr+2));
    //Vec3d n(*(ptr+3), *(ptr+4), *(ptr+5));

    auto Self = (Triangulator*)data;
    if (Self->addNormals) Self->geo->pushVert(p, Vec3d(0,1,0));
    else Self->geo->pushVert(p);
    Self->num_points++;
    //Self->geo->pushVert(p, n);
    //cout << "vert " << p << endl;
}

void tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4], const GLfloat neighborWeight[4], GLdouble **outV, void* data) {
    auto Self = (Triangulator*)data;
    Vec3d p(newVertex[0], newVertex[1], newVertex[2]);
    Vec3d c(0,0,0);
    Self->tmpVertices.push_back(p); // need to be stored locally
    Self->tmpVertices.push_back(c); // need color
    *outV = &Self->tmpVertices[Self->tmpVertices.size()-2][0];
    //cout << "combine " << p << "  " << (*outV)[0] << endl;
}

void tessErrorCB(GLenum errorCode) {
    const GLubyte *errorStr;
    errorStr = gluErrorString(errorCode);
    cerr << "[ERROR]: " << errorStr << endl;
}

vector<Vec3d> Triangulator::toSpace(const vector<Vec2d>& poly) {
    for (auto& v : poly) {
        if (isNan(v)) {
            cout << "Warning in Triangulator::tessellate, Vec2d contains NaN: " << v << endl;
            return vector<Vec3d>();
        }
    }
    vector<Vec3d> res;
    for (auto& v : poly) res.push_back(Vec3d(v[0], 0, v[1]));
    return res;
}


void Triangulator::tessellate() {
    GLuint id = glGenLists(1);  // create a display list
    GLUtesselator *tess = gluNewTess(); // create a tessellator
    if (!id) { cout << " Warning in Triangulator::tessellate, failed to create GL list" << endl; return; }          // failed to create a list, return 0
    if (!tess) { cout << " Warning in Triangulator::tessellate, failed to create GLU tessellation" << endl; return; }        // failed to create tessellation object, return 0

    // register callback functions
    //current_triangulator = this;
    gluTessCallback(tess, GLU_TESS_BEGIN_DATA,   (void (*)()) tessBeginCB);
    gluTessCallback(tess, GLU_TESS_END_DATA,     (void (*)()) tessEndCB);
    gluTessCallback(tess, GLU_TESS_ERROR,        (void (*)()) tessErrorCB);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA,  (void (*)()) tessVertexCB);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (*)()) tessCombineCB);

    vector<vector<Vec3d> > bounds;
    for (auto b : outer_bounds) if (b.size2() > 2) bounds.push_back( toSpace(b.get()) );
    for (auto b : inner_bounds) if (b.size2() > 2) bounds.push_back( toSpace(b.get()) );
    for (auto b : outer_bounds) if (b.size3() > 2) bounds.push_back( b.get3() );
    for (auto b : inner_bounds) if (b.size3() > 2) bounds.push_back( b.get3() );

    if (bounds.size() > 0) {
        //cout << "tessellate " << toString(bounds) << endl;
        gluTessBeginPolygon(tess, (void*)this);
        for (auto& b : bounds) {
            if (b.size() <= 2) {
                cout << "Warning in Triangulator::tessellate, bound size below 3: " << b.size() << endl;
                continue;
            }
            gluTessBeginContour(tess);
            for (auto& v : b) gluTessVertex(tess, &v[0], &v[0]);
            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);
    }

    //tmpVertices.clear();
    gluDeleteTess(tess);
}
