#include "triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include <GL/glut.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>

#include <OpenSG/OSGGeoProperties.h>

using namespace std;
using namespace OSG;

Triangulator* current_triangulator;
vector<Vec3d> tmpVertices;

struct Triangulator::GeoData {
    // geo data
    GeoUInt32PropertyRecPtr types;
    GeoUInt32PropertyRecPtr lengths;
    GeoUInt32PropertyRecPtr indices;
    GeoPnt3fPropertyRecPtr pos;
    GeoVec3fPropertyRecPtr norms;

    // tmp vars
    int current_primitive = -1;
    int current_vertex_count = 0;

    GeoData() {
        types = GeoUInt32Property::create();
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
};

void Triangulator::testQuad() {
    polygon p1;
    p1.addPoint(Vec2f(-2,3));
    p1.addPoint(Vec2f(-2,0));
    p1.addPoint(Vec2f(2,0));
    p1.addPoint(Vec2f(2,3));

    polygon p2;
    p2.addPoint(Vec2f(-1,2));
    p2.addPoint(Vec2f(-1,1));
    p2.addPoint(Vec2f(1,1));
    p2.addPoint(Vec2f(1,2));

    add(p1);
    add(p2, false);
}

Triangulator::Triangulator() { /*testQuad();*/ }

void Triangulator::add(polygon p, bool outer) {
    if (outer) outer_bounds.push_back(p);
    else inner_bounds.push_back(p);
}

VRGeometryPtr Triangulator::compute() {
    tessellate();
    auto g = VRGeometry::create("tessellation");

    if (geo) {
        if (geo->valid()) {
            g->setTypes(geo->types);
            g->setLengths(geo->lengths);
            //g->setIndices(geo->indices);
            g->setPositions(geo->pos);
            g->setNormals(geo->norms);
        }
    }

    return g;
}

// GLU_TESS CALLBACKS
void tessBeginCB(GLenum which) {
    auto Self = current_triangulator;
    if (!Self->geo) Self->geo = new Triangulator::GeoData();
    Self->geo->current_primitive = which;
    Self->geo->types->addValue(which);
    cout << "beg " << which << endl;
}

void tessEndCB() {
    auto Self = current_triangulator;
    int Nprim = Self->geo->current_vertex_count;
    /*switch(Self->geo->current_primitive) {
        case 0x0000: break; // GL_POINTS
        case 0x0001: Nprim *= 2; break; // GL_LINES
        case 0x0002: Nprim = 1; break; // GL_LINE_LOOP
        case 0x0003: Nprim = 1; break; // GL_LINE_STRIP
        case 0x0004: Nprim *= 3; break; // GL_TRIANGLES
        case 0x0005: Nprim = 1; break; // GL_TRIANGLE_STRIP
        case 0x0006: Nprim = 1; break; // GL_TRIANGLE_FAN
        case 0x0007: Nprim *= 4; break; // GL_QUADS
        case 0x0008: Nprim = 1; break; // GL_QUAD_STRIP
        case 0x0009: Nprim = 1; break; // GL_POLYGON
    }*/

    for (int i=0; i<Nprim; i++) {
        Self->geo->indices->addValue( i );
    }

    Self->geo->lengths->addValue( Nprim );
    Self->geo->current_vertex_count = 0;
    cout << "end" << endl;
}

void tessVertexCB(const GLvoid *data) { // draw a vertex
    const GLdouble *ptr = (const GLdouble*)data;
    Pnt3f p(*ptr, *(ptr+1), *(ptr+2));

    auto Self = current_triangulator;
    Self->geo->pos->addValue( p );
    Self->geo->norms->addValue( Vec3f(0,0,1) );
    Self->geo->current_vertex_count++;
    cout << "vert " << p << endl;
}

void tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4], const GLfloat neighborWeight[4], GLdouble **outV) {
    Vec3d p(newVertex[0], newVertex[1], newVertex[2]);
    Vec3d c(0,0,0);
    tmpVertices.push_back(p); // need to be stored locally
    tmpVertices.push_back(c); // need color
    *outV = &tmpVertices[tmpVertices.size()-2][0];
    cout << "combine " << p << "  " << (*outV)[0] << endl;
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

    auto toSpace = [&](const vector<Vec2f>& poly) {
        vector<Vec3d> res;
        for (auto& v : poly) res.push_back(Vec3d(v));
        return res;
    };

    vector<vector<Vec3d> > bounds;
    for (auto b : outer_bounds) bounds.push_back( toSpace(b.get()) );
    for (auto b : inner_bounds) bounds.push_back( toSpace(b.get()) );

    gluTessBeginPolygon(tess, 0);
    for (auto& b : bounds) {
        gluTessBeginContour(tess);
        for (auto& v : b) gluTessVertex(tess, &v[0], &v[0]);
        gluTessEndContour(tess);
    }
    gluTessEndPolygon(tess);
    //tmpVertices.clear();
    gluDeleteTess(tess);
}
