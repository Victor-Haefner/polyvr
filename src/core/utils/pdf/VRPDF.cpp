#include "VRPDF.h"
#include "VRPDFPage.h"
#include "VRPDFData.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/Layer2D.h"

#ifdef _WIN32
#include <cairo/cairo-pdf.h>
#else
#include <cairo-pdf.h>
#endif

using namespace OSG;

VRPDF::VRPDF() {}
VRPDF::~VRPDF() {}
VRPDFPtr VRPDF::create() { return VRPDFPtr( new VRPDF() ); }

void VRPDF::read(string path) {
    auto data = VRPDFData::create();
    data->read(path);

}

void VRPDF::write(string path) {
    _cairo_surface* surface = cairo_pdf_surface_create(path.c_str(), 0, 0);
    if (!surface) return;
    _cairo* cr = cairo_create(surface);
    if (!cr) return;
    for (auto& page : pages) page->writeTo(cr);
    cairo_show_page(cr);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
}


int VRPDF::getNPages() { return pages.size(); }

VRPDFPagePtr VRPDF::addPage() {
    auto page = VRPDFPage::create(0); // TODO
    pages.push_back(page);
    return page;
}

VRPDFPagePtr VRPDF::getPage(int i) {
    if (i < 0) i += pages.size();
    if (i < 0 || i >= pages.size()) return 0;
    return pages[i];
}

void VRPDF::remPage(int i) {
    if (i < 0) i += pages.size();
    if (i < 0 || i >= pages.size()) return;
    pages.erase(pages.begin() + i);
}



