#include "VRPDF.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"

#include <cairo-pdf.h>

using namespace OSG;

template<> string typeName(const VRPDF& p) { return "PDF"; }

VRPDF::VRPDF(string path) {
    //cairo_surface_t* surface = cairo_pdf_surface_create(path.c_str(), W*res, H*res);
    surface = cairo_pdf_surface_create(path.c_str(), 0, 0);
    cr = cairo_create(surface);
}

VRPDF::~VRPDF() {
    if (surface) cairo_surface_destroy(surface);
    if (cr) cairo_destroy(cr);
}

VRPDFPtr VRPDF::create(string path) { return VRPDFPtr( new VRPDF(path) ); }

void VRPDF::drawLine(Vec2d p1, Vec2d p2, Color3f c1, Color3f c2) {
    cairo_set_source_rgb(cr, c1[0], c1[1], c1[2]);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, p1[0]*res, p1[1]*res);
    cairo_set_source_rgb(cr, c2[0], c2[1], c2[2]);
    cairo_line_to(cr, p2[0]*res, p2[1]*res);
    cairo_stroke(cr);
}

void VRPDF::drawText() {
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 40.0);
    cairo_move_to(cr, 10.0, 50.0);
    cairo_show_text(cr, "Disziplin ist Macht.");
}

void VRPDF::write(string path) {
    cairo_show_page(cr);
}

Vec2d VRPDF::projectVector(Pnt3d v, PosePtr plane) {
    double x = Vec3d(v).dot(plane->x())  - Ox;
    double y = Vec3d(v).dot(plane->up()) - Oy;
    return Vec2d(x,y);
}

void VRPDF::projectGeometry(VRGeometryPtr geo, PosePtr plane, PosePtr transform) {
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
            drawLine(p1,p2,c,c);
        }
    }
}

void VRPDF::project(VRObjectPtr obj, PosePtr plane) {
    BoundingboxPtr bb = obj->getBoundingbox();
    W = abs( bb->size().dot(plane->x()) );
    H = abs( bb->size().dot(plane->up()) );
    Ox = min( bb->min().dot(plane->x()),  bb->max().dot(plane->x()) );
    Oy = min( bb->min().dot(plane->up()), bb->max().dot(plane->up()) );

    cout << "VRPDF::project, " << Vec4d(W,H,Ox,Oy) << endl;

    cairo_pdf_surface_set_size(surface, W*res, H*res);

    for (auto g : obj->getChildren(true, "Geometry", true)) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(g);
        //PosePtr transform = obj->getPoseTo(g);
        PosePtr transform = geo->getWorldPose();
        projectGeometry(geo, plane, transform);
    }
}

void VRPDF::slice(VRObjectPtr obj, PosePtr plane) {
    // TODO
}
