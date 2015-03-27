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

ImageRecPtr VRText::createBmp (string text, string font, int width, int height, Color4f c, Color4f bg) {

    //Cairo
    cairo_t *cr;
    cairo_surface_t *surface;

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
    cairo_rectangle(cr, 0.0, 0.0, width, height);
    cairo_set_source_rgba(cr, bg[0]/255.f, bg[1]/255.f, bg[2]/255.f, bg[3]/255.f);
    cairo_fill(cr);

    //text
    cairo_set_source_rgba (cr, c[0]/255.f, c[1]/255.f, c[2]/255.f, c[3]/255.f);
    pango_cairo_update_layout (cr, layout);
    pango_cairo_show_layout (cr, layout);


    UChar8* data = cairo_image_surface_get_data(surface);
    convertData(data, width, height);

    ImageRecPtr img = Image::create();
    img->set( Image::OSG_BGRA_PF, width, height, 1, 1, 1, 0, data);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return img;

}

/*void setImage(SimpleTexturedMaterialRecPtr tex, ImageRecPtr img) {
    beginEditCP(tex);
        tex->setImage(img);
        tex->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
        tex->setMagFilter(GL_LINEAR);
        //ES MÜSSEN NOCH ALLE PARAMETER GESETZT WERDEN!!
    endEditCP(tex);
}*/

/*void write_png() {
    png::image< png::rgba_pixel > image(128, 128);
    for (size_t y = 0; y < image.get_height(); ++y)
    {
        for (size_t x = 0; x < image.get_width(); ++x)
        {
            image[y][x] = png::rgba_pixel(x, y, x + y, 255);
            // non-checking equivalent of image.set_pixel(x, y, ...);
        }
    }
    image.write("rgb.png");
    cout << "\n\nWRITE!!\n\n";
}*/

VRText* VRText::get() {
    static VRText* singleton_opt = new VRText();
    return singleton_opt;
}

ImageRecPtr VRText::create(string text, string font, int scale, Color4f fg, Color4f bg) {
    int l = text.size();
    return createBmp(text, font, scale*l, scale*1.5, fg, bg);
}

SimpleTexturedMaterialRecPtr VRText::getTexture (string text, string font, int scale, Color4f fg, Color4f bg) {
    SimpleTexturedMaterialRecPtr tex = SimpleTexturedMaterial::create();
    int l = text.size();
    tex->setImage( createBmp(text, font, scale*l, scale*1.5, fg, bg) );
    return tex;
}


OSG_END_NAMESPACE;
