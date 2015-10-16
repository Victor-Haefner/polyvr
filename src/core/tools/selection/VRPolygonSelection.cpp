#include "VRPolygonSelection.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

VRPolygonSelection::VRPolygonSelection() {
    auto mat = VRMaterial::create("PolygonSelection_mat");
    mat->setDiffuse(Vec3f(0.5,0.5,1));
    mat->setLit(0);
    mat->setLineWidth(3);
    shape = VRGeometry::create("PolygonSelection");
    shape->setPersistency(0);
    shape->setMaterial(mat);
    selection.setNearFar(Vec2f(0.1,1000));
}

shared_ptr<VRPolygonSelection> VRPolygonSelection::create() { return shared_ptr<VRPolygonSelection>( new VRPolygonSelection() ); }

void VRPolygonSelection::setOrigin(pose orig) { selection.setPose(orig); }
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

void VRPolygonSelection::addEdge(Vec3f dir) {
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

bool VRPolygonSelection::objSelected(VRGeometryPtr geo) {
    if (!closed) return false;

    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    boundingbox bbox;
    bbox.update(v1);
    bbox.update(v2);

    Vec3f p0 = origin.pos();
    for (auto d : selection.getEdges()) {
        if ( bbox.intersectedBy( Line(p0,d) ) ) {
            return true;
        }
    }
    return false;
}

bool VRPolygonSelection::partialSelected(VRGeometryPtr geo) {
    if (!closed) return false;

    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    boundingbox bbox;
    bbox.update(v1);
    bbox.update(v2);

    Vec3f p0 = origin.pos();
    for (auto d : selection.getEdges()) if ( bbox.intersectedBy( Line(p0,d) ) ) return true;
    return false;
}

bool VRPolygonSelection::vertSelected(Vec3f p) {
    if (!closed) return false;

    auto inFrustum = [&](frustum& f) {
        auto planes = f.getPlanes();
        for (int i=0; i<planes.size(); i++) {
            float d = planes[i].distance(p);
            if ( d < 0 ) return false;
        }
        return true;
    };

    if (!inFrustum(convex_hull)) return false;

    for (auto f : convex_decomposition ) if (inFrustum(f)) return true;

    return false;
}

void VRPolygonSelection::updateShape(frustum f) {
    int N = f.getEdges().size();
    if (N <= 1) return;

    auto trans = f.getPose();
    Vec3f dir = trans.dir(); dir.normalize();
    Vec3f p0 = trans.pos();
    float near = 1;
    float far = 1;
    if (!bbox.empty()) near = dir.dot( bbox.center() - p0 ) - bbox.radius();
    if (!bbox.empty()) far = dir.dot( bbox.center() - p0 ) + bbox.radius();

    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr types = GeoUInt32Property::create();

    for (auto e : f.getEdges()) {
        pos->addValue(p0+e*near);
        pos->addValue(p0+e*far);
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





