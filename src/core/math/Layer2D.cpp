#include "Layer2D.h"

#include "core/math/pose.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

Layer2D::Layer2D() {}
Layer2D::~Layer2D() {}

Layer2D::Line::Line(Pnt2d p1, Pnt2d p2, Color3f c1, Color3f c2) : p1(p1), p2(p2), c1(c1), c2(c2) {}

vector<Layer2D::Line>& Layer2D::getLines() { return lines; }

Vec2d Layer2D::projectVector(Pnt3d v, PosePtr plane) {
    double x = Vec3d(v).dot(plane->x());
    double y = Vec3d(v).dot(plane->up());
    return Vec2d(x,y);
}

void Layer2D::projectGeometry(VRGeometryPtr geo, PosePtr plane, PosePtr transform) {
    Color3f c = geo->getMaterial()->getDiffuse();
    VRGeoData data(geo);
    for (auto prim : data) {
        if (prim.type == GL_LINES) {
            size_t i1 = prim.indices[0];
            size_t i2 = prim.indices[1];
            Pnt3d P1 = transform->transform( Vec3d(data.getPosition(i1)) );
            Pnt3d P2 = transform->transform( Vec3d(data.getPosition(i2)) );
            Vec2d p1 = projectVector(P1, plane);
            Vec2d p2 = projectVector(P2, plane);
            lines.push_back(Line(p1,p2,c,c));
        }
    }
}

void Layer2D::project(VRObjectPtr obj, PosePtr plane) {
    for (auto g : obj->getChildren(true, "Geometry", true)) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(g);
        PosePtr transform = geo->getWorldPose();
        projectGeometry(geo, plane, transform);
    }
}

void Layer2D::slice(VRObjectPtr obj, PosePtr plane) {
    // TODO
}
