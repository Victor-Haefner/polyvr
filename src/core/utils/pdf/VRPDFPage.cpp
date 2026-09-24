#include "VRPDFPage.h"
#include "VRPDF3DModel.h"

#include "core/objects/object/VRObject.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/pose.h"

#ifdef _WIN32
#include <cairo/cairo-pdf.h>
#else
#include <cairo-pdf.h>
#endif

using namespace OSG;

VRPDFPage::VRPDFPage(VRPDFDataPtr data) : data(data) {}
VRPDFPage::~VRPDFPage() {}

VRPDFPagePtr VRPDFPage::create(VRPDFDataPtr data) { return VRPDFPagePtr( new VRPDFPage(data) ); }
VRPDFPagePtr VRPDFPage::ptr() { return static_pointer_cast<VRPDFPage>(shared_from_this()); }

//TODO
void VRPDFPage::writeTo(_cairo* cr) {
    ;
}

//TODO
void VRPDFPage::drawLine(Pnt2d p1, Pnt2d p2, Color3f c1, Color3f c2) {
    /*cairo_set_source_rgb(cr, c1[0], c1[1], c1[2]);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, p1[0]*res, p1[1]*res);
    cairo_set_source_rgb(cr, c2[0], c2[1], c2[2]);
    cairo_line_to(cr, p2[0]*res, p2[1]*res);
    cairo_stroke(cr);*/
}

//TODO
void VRPDFPage::drawText() {
    /*cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 40.0);
    cairo_move_to(cr, 10.0, 50.0);
    cairo_show_text(cr, "Disziplin ist Macht.");*/
}

//TODO
void VRPDFPage::project(VRObjectPtr obj, PosePtr plane) {
    /*BoundingboxPtr bb = obj->getBoundingbox();
    W = abs( bb->size().dot(plane->x()) );
    H = abs( bb->size().dot(plane->up()) );
    Ox = min( bb->min().dot(plane->x()),  bb->max().dot(plane->x())  );
    Oy = min( bb->min().dot(plane->up()), bb->max().dot(plane->up()) );

    cout << "VRPDFPage::project, " << Vec4d(W,H,Ox,Oy) << endl;
    Vec2d O = Vec2d(Ox,Oy);

    cairo_pdf_surface_set_size(surface, W*res, H*res);
    Layer2D projection;
    projection.project(obj, plane);
    for (auto l : projection.getLines()) drawLine(l.p1-O, l.p2-O, l.c1, l.c2);*/
}

void VRPDFPage::slice(VRObjectPtr obj, PosePtr plane) {
    // TODO
}




vector<VRTechnicalDrawingPtr> VRPDFPage::extract2DModels() {
    vector<VRTechnicalDrawingPtr> res;
    return res;
}

vector<VRTransformPtr> VRPDFPage::extract3DModels() {
    vector<VRTransformPtr> res;

    VRPDF3DModel model;
    return model.extract(data->objects, data->buffer);
    //return res;
}
