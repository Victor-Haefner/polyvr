#include "VRPolygonSelection.h"
#include "core/math/boundingbox.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

VRPolygonSelection::VRPolygonSelection() {
    auto mat = VRMaterial::create("VRPolygonSelection_mat");
    mat->setDiffuse(Color3f(0.5,0.5,1));
    mat->setLit(0);
    mat->setLineWidth(3);
    shape = VRGeometry::create("VRPolygonSelection");
    shape->setPersistency(0);
    shape->setMaterial(mat);
    selection.setNearFar(Vec2d(0.1,1000));
}

shared_ptr<VRPolygonSelection> VRPolygonSelection::create() { return shared_ptr<VRPolygonSelection>( new VRPolygonSelection() ); }

void VRPolygonSelection::setOrigin(Pose orig) {
    selection.setPose(orig);
    origin = orig;
}

VRGeometryPtr VRPolygonSelection::getShape() { return shape; }
bool VRPolygonSelection::isClosed() { return closed; }

void VRPolygonSelection::close(VRObjectPtr world) {
    selection.close();
    convex_hull = selection.getConvexHull();
    convex_hull.close();
    convex_decomposition = selection.getConvexDecomposition();
    closed = true;
    apply(world);
    updateSubselection();
    updateShape(selection);
}

void VRPolygonSelection::addEdge(Vec3d dir) {
    if (closed) clear();
    selection.addEdge(dir);
    updateShape(selection);
}

void VRPolygonSelection::clear() {
    shape->hide();
    VRSelection::clear();
    selection.clear();
    convex_hull.clear();
    convex_decomposition.clear();
    closed = false;
}

bool inFrustum(Frustum& f, Vec3f p) {
    auto planes = f.getPlanes();
    for (unsigned int i=0; i<planes.size(); i++) {
        float d = planes[i].distance(p);
        if ( d < 0 ) return false;
    }
    return true;
}

bool VRPolygonSelection::objSelected(VRGeometryPtr geo) {
    if (!closed) return false;
    auto bbox = geo->getBoundingbox();
    Vec3d p0 = origin.pos();
    Vec3f c = Vec3f(bbox->center());
    if (!inFrustum(convex_hull, c)) return false;
    for (auto f : convex_decomposition ) if (inFrustum(f, c)) return true;
    for (auto d : selection.getEdges()) {
        if ( bbox->intersectedBy( Line(Pnt3f(p0),Vec3f(d)) ) ) return true;
    }
    return false;
}

Frustum VRPolygonSelection::getSelectionFrustum() { return selection; }

bool VRPolygonSelection::partialSelected(VRGeometryPtr geo) {
    if (!closed) return false;
    auto bbox = geo->getBoundingbox();
    Vec3d p0 = origin.pos();
    for (auto d : selection.getEdges()) if ( bbox->intersectedBy( Line(Pnt3f(p0),Vec3f(d)) ) ) return true;
    return false;
}

bool VRPolygonSelection::vertSelected(Vec3d p) {
    if (!closed) return false;
    if (!inFrustum(convex_hull, Vec3f(p))) return false;
    for (auto f : convex_decomposition ) if (inFrustum(f, Vec3f(p))) return true;
    return false;
}

void VRPolygonSelection::updateShape(Frustum f) {
    int N = f.getEdges().size();
    if (N <= 1) return;

    auto trans = f.getPose();
    Vec3d dir = trans.dir(); dir.normalize();
    Vec3d p0 = trans.pos();
    float Near = 1;
    float Far = 1;
    if (bbox && !bbox->empty()) Near = dir.dot( bbox->center() - p0 ) - bbox->radius();
    if (bbox && !bbox->empty()) Far = dir.dot( bbox->center() - p0 ) + bbox->radius();

    GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
    GeoUInt32PropertyMTRecPtr types = GeoUInt32Property::create();

    cout << "updateShape" << endl;
    for (auto e : f.getEdges()) {
        pos->addValue(p0+e*Near);
        pos->addValue(p0+e*Far);
        cout << " p0 " << p0 << " e " << e << " nf " << Near << " " << Far << endl;
    }

    // near
    lengths->addValue(N);
    types->addValue(GL_LINE_STRIP);
    for (int i=0; i<N; i++) inds->addValue(i*2);

    // far
    lengths->addValue(N);
    types->addValue(GL_LINE_STRIP);
    for (int i=0; i<N; i++) inds->addValue(i*2+1);

    // sides
    lengths->addValue(N*2);
    types->addValue(GL_LINES);
    for (int i=0; i<N; i++) {
        inds->addValue(i*2);
        inds->addValue(i*2+1);
    }

    //shape->clear();
    shape->setTypes(types);
    shape->setPositions(pos);
    shape->setIndices(inds);
    shape->setLengths(lengths);
    shape->show();
}





