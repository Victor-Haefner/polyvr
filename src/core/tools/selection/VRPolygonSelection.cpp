#include "VRPolygonSelection.h"
#include "core/objects/geometry/VRGeometry.h"
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

VRPolygonSelection::VRPolygonSelection() {
    shape = VRGeometry::create("PolygonSelection");
}

shared_ptr<VRPolygonSelection> VRPolygonSelection::create() { return shared_ptr<VRPolygonSelection>( new VRPolygonSelection() ); }

void VRPolygonSelection::setOrigin(pose orig) { selection.setPose(orig); }
void VRPolygonSelection::addEdge(Vec3f dir) { if (!closed) selection.addEdge(dir); }
VRGeometryPtr VRPolygonSelection::getShape() { return shape; }
void VRPolygonSelection::close() {
    selection.close();
    convex_hull = selection.getConvexHull();
    convex_hull.close();
    convex_decomposition = selection.getConvexDecomposition();
    closed = true;

    //updateShape(selection);
    updateShape(convex_hull);
}

void VRPolygonSelection::clear() {
    VRSelection::clear();
    selection.clear();
    convex_hull.clear();
    convex_decomposition.clear();
    closed = false;
}

bool VRPolygonSelection::objSelected(VRGeometryPtr geo) {
    if (!closed) return false;

    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    vector<Vec3f> corners;
    corners.push_back(v1);
    corners.push_back(Vec3f(v1[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v1[0], v2[1], v2[2]));
    corners.push_back(v2);
    corners.push_back(Vec3f(v1[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v1[2]));

    for (auto c : corners) if (!vertSelected(c)) return false;
    return true;
}

bool VRPolygonSelection::partialSelected(VRGeometryPtr geo) {
    if (!closed) return false;

    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    vector<Vec3f> corners;
    corners.push_back(v1);
    corners.push_back(Vec3f(v1[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v1[0], v2[1], v2[2]));
    corners.push_back(v2);
    corners.push_back(Vec3f(v1[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v1[2]));

    for (auto c : corners) if (vertSelected(c)) return true;
    return false;
}

bool VRPolygonSelection::vertSelected(Vec3f p) {
    if (!closed) return false;

    auto planes = convex_hull.getPlanes();
    //cout << endl;
    for (int i=0; i<planes.size(); i++) {
        float d = planes[i].distance(p);
        //cout << "vertSelected i" << i << " p " << p << " d " << d << " pl " << planes[i] << " pls " << planes.size() << endl;
        if ( d < 0 ) return false;
    }

    /*for (auto f : convex_decomposition ) {
        for (Plane pl : f.getPlanes()) if ( pl.distance(p) < 0) return true;
    }*/

    return true;
}

void VRPolygonSelection::updateShape(frustum f, bool transform) {
    if (f.getEdges().size() == 0) return;

    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

    auto trans = f.getPose();
    Matrix m = trans.asMatrix();
    pos->addValue(trans.pos());
    for (auto e : f.getEdges()) {
        inds->addValue(0);
        inds->addValue(pos->size());
        if (transform) m.mult(e,e);
        pos->addValue(trans.pos()+e*3);
    }

    //shape->clear();
    shape->setType(GL_LINES);
    shape->setPositions(pos);
    shape->setIndices(inds);
}





