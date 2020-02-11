#include "VRText.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>
#include <new>
#include <codecvt>

#ifndef WITHOUT_PANGO_CAIRO
//flags mit  $ pkg-config --cflags pango und $ pkg-config --libs pango :)
#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <pango/pangocairo.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H


#include "core/objects/material/VRTexture.h"
#include "core/scene/VRSceneManager.h"


using namespace std;
using namespace OSG;

class FTUtils {
    private:
        FT_Library    library;
        FT_Face       face;
        FT_GlyphSlot  slot;
        FT_Matrix     matrix;                 /* transformation matrix */
        FT_Vector     pen;                    /* untransformed origin  */
        FT_Error      error;

        float WIDTH = 100;
        float HEIGHT = 20;

        vector<unsigned char> image;

    public:
        FTUtils() : image(WIDTH*HEIGHT,0) {
            clear_img();

            int e = FT_Init_FreeType( &library );
            if (e) cout << "FT_Init_FreeType failed!" << endl;

            string font = VRSceneManager::get()->getOriginalWorkdir();
            font += "/ressources/fonts/Mono.ttf";
            e = FT_New_Face( library, font.c_str(), 0, &face );
            if (e) cout << "FT_New_Face failed!" << endl;

            char*         filename;
            char*         text;

            double        angle;
            int           n, num_chars;

                int X = 0;
                int Y = 0;
                int S = 20;

            text          = "A-ö-ß";                           /* second argument    */


              /* use 50pt at 100dpi */
              error = FT_Set_Char_Size( face, S * 64, 0, 100, 0 );                /* set character size */
              /* error handling omitted */

              /* cmap selection omitted;                                        */
              /* for simplicity we assume that the font contains a Unicode cmap */

              slot = face->glyph;

              /* set up matrix */
              matrix.xx = (FT_Fixed)( 0x10000L );
              matrix.xy = (FT_Fixed)( 0 );
              matrix.yx = (FT_Fixed)( 0 );
              matrix.yy = (FT_Fixed)( 0x10000L );

              /* the pen position in 26.6 cartesian space coordinates; */
              /* start at (300,200) relative to the upper left corner  */
                pen.x = X * 64;
                pen.y = ( HEIGHT - Y - S ) * 64;

                // TODO: use this for countGraphemes!
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
                const std::wstring wideText = convert.from_bytes(text);
                num_chars = wideText.size();

              for ( n = 0; n < num_chars; n++ ) {
                /* set transformation */
                FT_Set_Transform( face, &matrix, &pen );

                /* load glyph image into the slot (erase previous one) */
                FT_ULong cUL = wideText[n];
                //int s = 1;
                //for (int i = 0; i<chars[n].size(); i++) { cUL += chars[n][i]*s; s*=256; }
                error = FT_Load_Char( face, cUL, FT_LOAD_RENDER );
                if ( error ) {
                    cout << "FT_Load_Char " << cUL << " failed! " << error << endl;
                    continue;                 /* ignore errors */
                }

                /* now, draw to our target surface (convert position) */
                draw_bitmap( &slot->bitmap, slot->bitmap_left, HEIGHT - slot->bitmap_top );

                /* increment pen position */
                pen.x += slot->advance.x;
                pen.y += slot->advance.y;
              }

              show_image();

              FT_Done_Face    ( face );
              FT_Done_FreeType( library );

        }

        void set(int x, int y, int v) { image[x+y*WIDTH] = v; }
        int get(int x, int y) { return image[x+y*WIDTH]; }

        void clear_img() {
            for ( int i = 0; i < WIDTH; i++ ) {
                for ( int j = 0; j < HEIGHT; j++ ) set(i,j,0);
            }
        }

        void draw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y) {
              FT_Int  i, j, p, q;
              FT_Int  x_max = x + bitmap->width;
              FT_Int  y_max = y + bitmap->rows;


              /* for simplicity, we assume that `bitmap->pixel_mode' */
              /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

              for ( i = x, p = 0; i < x_max; i++, p++ ) {
                for ( j = y, q = 0; j < y_max; j++, q++ ) {
                  if ( i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT ) continue;
                  set(i,j, bitmap->buffer[q * bitmap->width + p]);
                }
              }
        }

        void show_image( void ) {
            ofstream fft("testFT.txt");

            for ( int j = 0; j < WIDTH; j++ ) fft << "-";
            fft << endl;

            for ( int j = 0; j < HEIGHT; j++ ) {
                for ( int i = 0; i < WIDTH; i++ ) {
                    int B = get(i,j);
                    char c = B == 0 ? ' ' : B < 128 ? '+' : '*';
                    fft << c;
                }
                fft << endl;
            }

            for ( int j = 0; j < WIDTH; j++ ) fft << "-";
            fft << endl;
        }
};





VRText* VRText::get() {
    static VRText* singleton_opt = new VRText();
    return singleton_opt;
}

VRTexturePtr VRText::create(string text, string font, int res, Color4f fg, Color4f bg) {
    resolution = res;
    return createBmp(text, font, fg, bg);
}

void VRText::analyzeText() {
    Nlines = 0;
    maxLineLength = 0;

    stringstream ss(text);
    for (string line; getline(ss, line); ) {
        maxLineLength = max(maxLineLength, countGraphemes(line));
        Nlines++;
    }
}

void VRText::computeTexParams() {
    texWidth = resolution*maxLineLength;
    texHeight = resolution*1.5;
    texWidth += 2*padding;
    texHeight += 2*padding;
    texHeight *= Nlines;
}

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

    delete[] buffer;
}

VRTexturePtr VRText::createBmp (string text, string font, Color4f fg, Color4f bg) {
#ifdef WITHOUT_PANGO_CAIRO
    FTUtils ft;
    //ft.
    VRTexturePtr tex = VRTexture::create();
    return tex;
#else
    this->text = text;
    analyzeText();
    computeTexParams();

    cairo_surface_t* surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, texWidth, texHeight);
    cairo_t* cr = cairo_create (surface);
    PangoLayout* layout = pango_cairo_create_layout (cr);
    pango_layout_set_text(layout, text.c_str(), -1);
    //pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);//MACHT IRGENDWIE NIX

    //Pango Description
    PangoFontDescription* desc = pango_font_description_from_string (font.c_str());
    pango_layout_set_font_description (layout, desc);
    pango_layout_get_pixel_size(layout, &layoutWidth, &layoutHeight);
    pango_font_description_free (desc);

    //hier wird gemalt!
    //background
    cairo_set_source_rgba(cr, bg[0],bg[1],bg[2],bg[3]);
    cairo_rectangle(cr, 0, 0, texWidth, texHeight);
    cairo_fill(cr);

    //text
    cairo_set_source_rgba (cr, fg[0],fg[1],fg[2],fg[3]);
    cairo_translate(cr, padding, padding);
    pango_cairo_update_layout (cr, layout);
    pango_cairo_show_layout (cr, layout);

    cairo_set_source_rgba(cr, bg[0],bg[1],bg[2],bg[3]);
    cairo_rectangle(cr, 0, 0, texWidth, padding-1); cairo_fill(cr);
    cairo_rectangle(cr, 0, texHeight-padding-1, texWidth, padding-1); cairo_fill(cr);
    cairo_rectangle(cr, 0, 0, padding-1, texHeight); cairo_fill(cr);
    cairo_rectangle(cr, texWidth-padding-1, 0, padding-1, texHeight); cairo_fill(cr);

    UChar8* data = cairo_image_surface_get_data(surface);
    convertData(data, texWidth, texHeight);

    VRTexturePtr tex = VRTexture::create();
    tex->getImage()->set( Image::OSG_BGRA_PF, texWidth, texHeight, 1, 1, 1, 0, data);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    return tex;
#endif
}

#ifndef WITHOUT_UNICODE
#include <unicode/utypes.h>
#include <unicode/ubrk.h>
#include <unicode/utext.h>

struct UTextDeleter { void operator()(UText* ptr) { utext_close(ptr); } };
struct UBreakIteratorDeleter { void operator()(UBreakIterator* ptr) { ubrk_close(ptr); } };
using PUText = std::unique_ptr<UText, UTextDeleter>;
using PUBreakIterator = std::unique_ptr<UBreakIterator, UBreakIteratorDeleter>;

void checkStatus(const UErrorCode status) {
    if (U_FAILURE(status)) cout << u_errorName(status) << endl;
}

size_t VRText::countGraphemes(string txt) {
    /* TODO: try to get rid of unicode dependency!
    PangoContext* ctx = pango_context_new();
    GList* items = pango_itemize(ctx, txt.c_str(), 0, txt.size(), 0, 0);
    g_object_unref(ctx);

    GList *elem;
    //MyType *item;

    size_t charCount = 0;
    for(elem = items; elem; elem = elem->next) {
        //item = elem->data;
        charCount++;
    }
    return charCount;*/


    UErrorCode status = U_ZERO_ERROR;
    PUText text(utext_openUTF8(nullptr, txt.data(), txt.length(), &status));
    checkStatus(status);

    PUBreakIterator it(ubrk_open(UBRK_CHARACTER, "en_us", nullptr, 0, &status));
    checkStatus(status);
    ubrk_setUText(it.get(), text.get(), &status);
    checkStatus(status);
    size_t charCount = 0;
    while (ubrk_next(it.get()) != UBRK_DONE) charCount++;
    return charCount;
}

vector<string> VRText::splitGraphemes(string txt) {
    UErrorCode status = U_ZERO_ERROR;
    PUText text(utext_openUTF8(nullptr, txt.data(), txt.length(), &status));
    checkStatus(status);

    PUBreakIterator it(ubrk_open(UBRK_CHARACTER, "en_us", nullptr, 0, &status));
    checkStatus(status);
    ubrk_setUText(it.get(), text.get(), &status);
    checkStatus(status);
    vector<string> res;
    while (true) {
        UBreakIterator* IT = it.get();
        int i = ubrk_current(IT);
        if (ubrk_next(it.get()) == UBRK_DONE) break;
        UText* t = text.get();
        UChar32 c = utext_char32At(t, i);
        icu::UnicodeString uni_str(c);
        std::string str;
        uni_str.toUTF8String(str);
        res.push_back(str);
    }
    return res;
}
#else
size_t VRText::countGraphemes(string txt) {
    return txt.size();
}

vector<string> VRText::splitGraphemes(string txt) {
    vector<string> res;
    for (int i=0; i<txt.size(); i++) {
        string s(1,txt[i]);
        res.push_back(s);
    }
    return res;
}
#endif




