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

#include <ft2build.h>
#include FT_FREETYPE_H


#include "core/objects/material/VRTexture.h"
#include "core/scene/VRSceneManager.h"


using namespace std;
using namespace OSG;

class FTRenderer {
    private:
        FT_Library    library;
        FT_Face       face;
        FT_GlyphSlot  slot;
        FT_Matrix     matrix;                 /* transformation matrix */
        FT_Vector     pen;                    /* untransformed origin  */
        FT_Error      error;

        Vec4ub convrt(Color4f c) { return Vec4ub(c[0]*255,c[1]*255,c[2]*255,c[3]*255); }

    public:
        float WIDTH = 100;
        float HEIGHT = 20;
        Vec4ub background;
        vector<Vec4ub> image;

    public:
        FTRenderer(float width, float height, Color4f bg) : WIDTH(width), HEIGHT(height), background(convrt(bg)), image(width*height,background) {
            /* set up matrix */
            matrix.xx = (FT_Fixed)( 0x10000L );
            matrix.xy = (FT_Fixed)( 0 );
            matrix.yx = (FT_Fixed)( 0 );
            matrix.yy = (FT_Fixed)( 0x10000L );
        }

        static std::vector<pair<unsigned long, string>> getGraphemes(const std::string& utf8) {
            std::vector<pair<unsigned long, string>> unicode;
            for (int i=0; i<utf8.size();) {
                int i0 = i;
                unsigned long uni;
                size_t todo;
                unsigned char ch = utf8[i++];
                if (ch <= 0x7F) { uni = ch; todo = 0; } else if (ch <= 0xBF) return unicode;
                else if (ch <= 0xDF) { uni = ch&0x1F; todo = 1; }
                else if (ch <= 0xEF) { uni = ch&0x0F; todo = 2; }
                else if (ch <= 0xF7) { uni = ch&0x07; todo = 3; }
                else return unicode;

                for (size_t j = 0; j < todo; ++j) {
                    if (i == utf8.size()) return unicode;
                    unsigned char ch = utf8[i++];
                    if (ch < 0x80 || ch > 0xBF) return unicode;
                    uni <<= 6;
                    uni += ch & 0x3F;
                }
                if (uni >= 0xD800 && uni <= 0xDFFF) return unicode;
                if (uni > 0x10FFFF) return unicode;
                string str(utf8.substr(i0,i-i0));
                unicode.push_back(make_pair(uni, str));
            }

            return unicode;
        }

        // font is for example "Mono.ttf"
        int render(string text, string font, int resolution, int padding, Color4f fg) {
            error = FT_Init_FreeType( &library );
            if (error) cout << "FT_Init_FreeType failed!" << endl;

#ifndef __EMSCRIPTEN__
            font = VRSceneManager::get()->getOriginalWorkdir();
            font += "/ressources/fonts/Mono.ttf";
#endif

            error = FT_New_Face( library, font.c_str(), 0, &face );
            if (error) cout << "FT_New_Face failed!" << endl;

            int n, num_chars;
            int X = padding;
            int Y = padding;
            int S = resolution;

            error = FT_Set_Char_Size( face, S * 64, 0, 100, 0 ); // set character size,  use 50pt at 100dpi
            if (error) cout << "FT_Set_Char_Size failed!" << endl;

            /* cmap selection omitted;                                        */
            /* for simplicity we assume that the font contains a Unicode cmap */

            slot = face->glyph;


            /* the pen position in 26.6 cartesian space coordinates; */
            /* start at (300,200) relative to the upper left corner  */
            pen.x = X * 64;
            pen.y = ( HEIGHT - Y - S ) * 64;

            // TODO: use this for countGraphemes!
            auto graphemes = getGraphemes(text);
            num_chars = graphemes.size();
            //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
            //const std::wstring wideText = convert.from_bytes(text);
            //num_chars = wideText.size();

            int layoutWidth = 0;

            for ( n = 0; n < num_chars; n++ ) {

                FT_Set_Transform( face, &matrix, &pen );

                /* load glyph image into the slot (erase previous one) */
                FT_ULong cUL = graphemes[n].first;
                //int s = 1;
                //for (int i = 0; i<chars[n].size(); i++) { cUL += chars[n][i]*s; s*=256; }
                error = FT_Load_Char( face, cUL, FT_LOAD_RENDER );
                if ( error ) {
                    cout << "FT_Load_Char " << cUL << " failed! " << error << endl;
                    continue;                 /* ignore errors */
                }

                /* now, draw to our target surface (convert position) */
                draw_bitmap( &slot->bitmap, slot->bitmap_left, HEIGHT - slot->bitmap_top, fg );

                /* increment pen position */
                pen.x += slot->advance.x;
                pen.y += slot->advance.y;
                layoutWidth += slot->advance.x/64;
            }

            FT_Done_Face    ( face );
            FT_Done_FreeType( library );
            return layoutWidth;
        }

        void draw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y, Color4f fg) {
            Vec4ub foreground = convrt(fg);

            FT_Int  i, j, p, q;
            FT_Int  x_max = x + bitmap->width;
            FT_Int  y_max = y + bitmap->rows;

            for ( i = x, p = 0; i < x_max; i++, p++ ) {
                for ( j = y, q = 0; j < y_max; j++, q++ ) {
                    if ( i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT ) continue;
                    int v = bitmap->buffer[q * bitmap->width + p];
                    float t = v/255.0;
                    int k = i+(HEIGHT-j-1)*WIDTH;
                    Vec4ub c = Vec4ub(255,255,255,255);
                    c[0] = foreground[0]*t+background[0]*(1.0-t);
                    c[1] = foreground[1]*t+background[1]*(1.0-t);
                    c[2] = foreground[2]*t+background[2]*(1.0-t);
                    c[3] = foreground[3]*t+background[3]*(1.0-t);
                    //c = fg*t + background*(1.0-t); // TODO: wiso sieht das kacke aus?? müsste doch das gleiche sein wie oben!?!
                    image[k] = c;
                }
            }
        }
};





VRText* VRText::get() {
    static VRText* singleton_opt = new VRText();
    return singleton_opt;
}

VRTexturePtr VRText::create(string text, string font, int res, int pad, Color4f fg, Color4f bg) {
    resolution = res;
    padding = pad;
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
    texWidth = resolution*maxLineLength*1.5;
    texHeight = resolution*1.5;
    texWidth += 2*padding;
    texHeight += 2*padding;
    texHeight *= Nlines;

    texWidth = osgNextPower2(texWidth);
    texHeight = osgNextPower2(texHeight);
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
    this->text = text;
    analyzeText();
    computeTexParams();

    FTRenderer ft(texWidth, texHeight, bg);
    layoutWidth = ft.render(text, font, resolution*1.5, padding, fg);
    layoutHeight = texHeight;
    VRTexturePtr tex = VRTexture::create();
    tex->getImage()->set( Image::OSG_RGBA_PF, texWidth, texHeight, 1, 1, 1, 0, (UInt8*)&ft.image[0]);
    //tex->write("testMonoFT.png");
    return tex;
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
    return FTRenderer::getGraphemes(txt).size();
}

vector<string> VRText::splitGraphemes(string txt) {
    vector<string> res;
    for (auto g : FTRenderer::getGraphemes(txt)) {
        res.push_back(g.second);
    }
    return res;
}
#endif




