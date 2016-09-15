#include "VRText.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>
#include <new>

//flags mit  $ pkg-config --cflags pango und $ pkg-config --libs pango :)
#include <glib.h>
#include <stdlib.h>
#include <gmodule.h>
#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <pango/pangocairo.h>

#include "core/objects/material/VRTexture.h"


OSG_BEGIN_NAMESPACE;
using namespace std;


void VRText::convertData(UChar8* data, int width, int height) {
    UChar8* buffer = new UChar8[height*width*4];
    int fullSize = height*width*4;//char sind 1 byte gros!
    int rowSize = width*4;

    //drehe die zeilen um
    for (int i = 0; i < height ; i++) {
        UChar8* dataRow = data + i*4*width;
        UChar8* bufferRow = buffer + (height-1-i)*4*width;
        memcpy(bufferRow, dataRow, rowSize);
    }

    //kopiere sie zurück nach data
    memcpy(data, buffer, fullSize);

    delete buffer;
}

VRTexturePtr VRText::createBmp (string text, string font, int width, int height, Color4f fg, Color4f bg) {
    //Cairo
    cairo_t *cr;
    cairo_surface_t *surface;

    int R = 3; // padding px
    width += 2*R;
    height += 2*R;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);

    //Pango Layout
    PangoLayout *layout;
    layout = pango_cairo_create_layout (cr);
    pango_layout_set_text(layout, text.c_str(), -1);
    //pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);//MACHT IRGENDWIE NIX

    //Pango Description
    PangoFontDescription *desc;
    desc = pango_font_description_from_string (font.c_str());
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);

    //hier wird gemalt!
    //background
    cairo_set_source_rgba(cr, bg[0],bg[1],bg[2],bg[3]);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    //text
    cairo_set_source_rgba (cr, fg[0],fg[1],fg[2],fg[3]);
    cairo_translate(cr, R, R);
    pango_cairo_update_layout (cr, layout);
    pango_cairo_show_layout (cr, layout);

    cairo_set_source_rgba(cr, bg[0],bg[1],bg[2],bg[3]);
    cairo_rectangle(cr, 0, 0, width, R-1); cairo_fill(cr);
    cairo_rectangle(cr, 0, height-R-1, width, R-1); cairo_fill(cr);
    cairo_rectangle(cr, 0, 0, R-1, height); cairo_fill(cr);
    cairo_rectangle(cr, width-R-1, 0, R-1, height); cairo_fill(cr);

    UChar8* data = cairo_image_surface_get_data(surface);
    convertData(data, width, height);

    VRTexturePtr tex = VRTexture::create();
    tex->getImage()->set( Image::OSG_BGRA_PF, width, height, 1, 1, 1, 0, data);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return tex;
}

VRText* VRText::get() {
    static VRText* singleton_opt = new VRText();
    return singleton_opt;
}

VRTexturePtr VRText::create(string text, string font, int scale, Color4f fg, Color4f bg) {
    int l = text.size();
    return createBmp(text, font, scale*l, scale*1.5, fg, bg);
}

OSG_END_NAMESPACE;
