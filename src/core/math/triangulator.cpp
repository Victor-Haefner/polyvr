#include "triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include <GL/glut.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;


Triangulator* current_triangulator;
vector<Vec3d> tmpVertices;

/*struct Triangulator::GeoData {
    // geo data
    GeoUInt8PropertyMTRecPtr types;
    GeoUInt32PropertyMTRecPtr lengths;
    GeoUInt32PropertyMTRecPtr indices;
    GeoPnt3fPropertyMTRecPtr pos;
    GeoVec3fPropertyMTRecPtr norms;

    // tmp vars
    int current_primitive = -1;
    int current_vertex_count = 0;

    GeoData() {
        types = GeoUInt8Property::create();
        lengths = GeoUInt32Property::create();
        indices = GeoUInt32Property::create();
        pos = GeoPnt3fProperty::create();
        norms = GeoVec3fProperty::create();
    }

    bool valid() {
        if (!types->size()) { cout << "Triangulator Error: no types!\n"; return false; }
        if (!lengths->size()) { cout << "Triangulator Error: no lengths!\n"; return false; }
        if (!pos->size()) { cout << "Triangulator Error: no pos!\n"; return false; }
        return true;
    }
};*/

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

void Triangulator::add(VRPolygon p, bool outer) {
    if (outer) outer_bounds.push_back(p);
    else inner_bounds.push_back(p);
}

VRGeometryPtr Triangulator::compute() {
    tessellate();
    return geo ? geo->asGeometry("tessellation") : 0;
}

void Triangulator::append(VRGeoDataPtr data) {
    geo = data;
    tessellate();
    geo = 0;
}

// GLU_TESS CALLBACKS
void tessBeginCB(GLenum which) {
    auto Self = current_triangulator;
    if (!Self->geo) Self->geo = VRGeoData::create();
    Self->geo->pushType(which);
    Self->num_points = 0;
    //cout << "beg " << which << endl;
}

void tessEndCB() {
    auto Self = current_triangulator;
    int Nprim = Self->num_points;

    int Ni0 = Self->geo->getNIndices();
    for (int i=0; i<Nprim; i++) Self->geo->pushIndex( Ni0 + i );

    Self->geo->pushLength(Nprim);
    //Self->geo->updateType(Self->current_primitive, Self->num_points);
    //cout << "end" << endl;
}

void tessVertexCB(const GLvoid *data) { // draw a vertex
    const GLdouble *ptr = (const GLdouble*)data;
    Pnt3d p(*ptr, *(ptr+1), *(ptr+2));
    //Vec3d n(*(ptr+3), *(ptr+4), *(ptr+5));

    auto Self = current_triangulator;
    Self->geo->pushVert(p, Vec3d(0,1,0));
    Self->num_points++;
    //Self->geo->pushVert(p, n);
    //cout << "vert " << p << endl;
}

void tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4], const GLfloat neighborWeight[4], GLdouble **outV) {
    Vec3d p(newVertex[0], newVertex[1], newVertex[2]);
    Vec3d c(0,0,0);
    tmpVertices.push_back(p); // need to be stored locally
    tmpVertices.push_back(c); // need color
    *outV = &tmpVertices[tmpVertices.size()-2][0];
    //cout << "combine " << p << "  " << (*outV)[0] << endl;
}

void tessErrorCB(GLenum errorCode) {
    const GLubyte *errorStr;
    errorStr = gluErrorString(errorCode);
    cerr << "[ERROR]: " << errorStr << endl;
}

void Triangulator::tessellate() {
    GLuint id = glGenLists(1);  // create a display list
    GLUtesselator *tess = gluNewTess(); // create a tessellator
    if(!id) return;          // failed to create a list, return 0
    if(!tess) return;         // failed to create tessellation object, return 0

    // register callback functions
    current_triangulator = this;
    gluTessCallback(tess, GLU_TESS_BEGIN,   (void (*)()) tessBeginCB);
    gluTessCallback(tess, GLU_TESS_END,     (void (*)()) tessEndCB);
    gluTessCallback(tess, GLU_TESS_ERROR,   (void (*)()) tessErrorCB);
    gluTessCallback(tess, GLU_TESS_VERTEX,  (void (*)()) tessVertexCB);
    gluTessCallback(tess, GLU_TESS_COMBINE, (void (*)()) tessCombineCB);

    auto toSpace = [&](const vector<Vec2d>& poly) {
        vector<Vec3d> res;
        for (auto& v : poly) res.push_back(Vec3d(v[0], 0, v[1]));
        return res;
    };

    vector<vector<Vec3d> > bounds;
    for (auto b : outer_bounds) if (b.size2() > 2) bounds.push_back( toSpace(b.get()) );
    for (auto b : inner_bounds) if (b.size2() > 2) bounds.push_back( toSpace(b.get()) );
    for (auto b : outer_bounds) if (b.size3() > 2) bounds.push_back( b.get3() );
    for (auto b : inner_bounds) if (b.size3() > 2) bounds.push_back( b.get3() );

    if (bounds.size() > 0) {
        gluTessBeginPolygon(tess, 0);
        for (auto& b : bounds) {
            gluTessBeginContour(tess);
            for (auto& v : b) gluTessVertex(tess, &v[0], &v[0]);
            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);
    }

    //tmpVertices.clear();
    gluDeleteTess(tess);
}
