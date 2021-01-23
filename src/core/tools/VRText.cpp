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
#include FT_STROKER_H


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

        Vec4ub convrt(Color4f c) { return Vec4ub(c[0]*255, c[1]*255, c[2]*255, c[3]*255); }
        Color4f convrt(Vec4ub c) { return Color4f(c[0]/255.0, c[1]/255.0, c[2]/255.0, c[3]/255.0); }

        bool debugLayout = false;

    public:
        TextLayout data;
        FontStyle style;

    public:
        FTRenderer() {
            /* set up matrix */
            matrix.xx = (FT_Fixed)( 0x10000L );
            matrix.xy = (FT_Fixed)( 0 );
            matrix.yx = (FT_Fixed)( 0 );
            matrix.yy = (FT_Fixed)( 0x10000L );
        }

        void setFont(string font) {
            #ifndef __EMSCRIPTEN__
                font = VRSceneManager::get()->getOriginalWorkdir();
                font += "/ressources/fonts/Mono.ttf";
            #endif
            style.font = font;
        }

        void clearImg() {
            data.image = vector<Vec4ub>(data.width * data.height, convrt(style.background));
        }

        static vector<pair<unsigned long, string>> getGraphemes(const string& utf8) {
            vector<pair<unsigned long, string>> unicode;
            for (size_t i=0; i<utf8.size();) {
                size_t i0 = i;
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

        void computeLayout(vector<pair<unsigned long, string>>& graphemes) {
            int layoutWidth = 0;
            int layoutHeight = 0;
            int maxH = 0;
            int maxO = 0;

            //cout << "computeLayout " << graphemes.size() << endl;
            for ( size_t n = 0; n < graphemes.size(); n++ ) {
                FT_ULong cUL = graphemes[n].first; // load glyph image into the slot (erase previous one)
                error = FT_Load_Char( face, cUL, FT_LOAD_RENDER );
                if (error) { cout << "FT_Load_Char " << cUL << " failed! " << error << endl; continue; } // ignore errors
                layoutWidth += slot->advance.x/64;
                layoutHeight += slot->advance.y/64;
                if (n < graphemes.size()-1) layoutWidth += style.charspread;
                maxO = max(maxO, (int)(slot->bitmap.rows-slot->bitmap_top));
                maxH = max(maxH, (int)(slot->bitmap.rows + maxO));

                //cout << "  slot adv: " << Vec2i(slot->advance.x/64, slot->advance.y/64);
                //cout << "  (rows, top, rMax): " << Vec3i(slot->bitmap.rows, slot->bitmap_top, maxH) << endl;
            }

            layoutHeight += maxH;
            data.width = layoutWidth + 2*style.padding + 2*style.outline;
            data.height = layoutHeight + 2*style.padding + 2*style.outline;
            data.lineOffset = maxO;
        }

        void drawOutline(vector<pair<unsigned long, string>>& graphemes) {
            pen.x = 0;
            pen.y = 0;

            FT_Stroker stroker;
            FT_Stroker_New(library, &stroker);
            FT_Stroker_Set(stroker, style.outline * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

            //cout << "drawOutline " << endl;
            for ( size_t n = 0; n < graphemes.size(); n++ ) {
                FT_ULong cUL = graphemes[n].first; // load glyph image into the slot (erase previous one)
                FT_Set_Transform( face, &matrix, &pen );
                //error = FT_Load_Glyph(face, cUL, FT_LOAD_DEFAULT);
                //if (error) { cout << "FT_Load_Char " << cUL << " failed! " << error << endl; continue; } // ignore errors

                FT_UInt glyphIndex = FT_Get_Char_Index(face, cUL);
                FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
                FT_Glyph glyph;
                FT_Get_Glyph(face->glyph, &glyph);
                FT_Glyph_StrokeBorder(&glyph, stroker, false, true);
                FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
                FT_BitmapGlyph bmGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

                int X = bmGlyph->left + style.padding + style.outline + n*style.charspread;
                int Y = data.height - data.lineOffset - bmGlyph->top - style.padding - style.outline;
                draw_bitmap( &bmGlyph->bitmap, X, Y, style.outlineColor );
                //pen.x += bmGlyph->bitmap.width*64;
                //pen.y += bmGlyph->bitmap.rows*64;

                pen.x += slot->advance.x;
                pen.y += slot->advance.y;
                //cout << " pen " << Vec2i(pen.x, pen.y) << endl;
            }
        }

        void drawGraphemes(vector<pair<unsigned long, string>>& graphemes) {
            pen.x = 0;
            pen.y = 0;

            //cout << "drawGraphemes " << endl;
            for ( size_t n = 0; n < graphemes.size(); n++ ) {
                FT_ULong cUL = graphemes[n].first; // load glyph image into the slot (erase previous one)
                FT_Set_Transform( face, &matrix, &pen );
                error = FT_Load_Char( face, cUL, FT_LOAD_RENDER );
                if (error) { cout << "FT_Load_Char " << cUL << " failed! " << error << endl; continue; } // ignore errors

                int X = slot->bitmap_left + style.padding + style.outline + n*style.charspread;
                int Y = data.height - data.lineOffset - slot->bitmap_top - style.padding - style.outline;
                draw_bitmap( &slot->bitmap, X, Y, style.foreground );
                pen.x += slot->advance.x;
                pen.y += slot->advance.y;
                //cout << " pen " << Vec2i(pen.x, pen.y) << "  slot: " << Vec2i(slot->bitmap_left, slot->bitmap_top) << endl;
            }
        }

        void render(string text) {
            error = FT_Init_FreeType( &library );
            if (error) cout << "FT_Init_FreeType failed!" << endl;

            setFont(style.font);

            error = FT_New_Face( library, style.font.c_str(), 0, &face );
            if (error) cout << "FT_New_Face failed!" << endl;
            error = FT_Set_Char_Size( face, style.ptSize * 64, 0, style.dpi, style.dpi );
            if (error) cout << "FT_Set_Char_Size failed!" << endl;

            slot = face->glyph;
            auto graphemes = getGraphemes(text); // TODO: use this for countGraphemes!
            computeLayout(graphemes);
            clearImg();
            if (style.outline > 0) drawOutline(graphemes);
            drawGraphemes(graphemes);

            FT_Done_Face    ( face );
            FT_Done_FreeType( library );

            if (debugLayout) draw_padding();
        }

        void draw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y, Color4f fg) {
            FT_Int  i, j, p, q;
            FT_Int  x_max = x + bitmap->width;
            FT_Int  y_max = y + bitmap->rows;

            for ( i = x, p = 0; i < x_max; i++, p++ ) {
                for ( j = y, q = 0; j < y_max; j++, q++ ) {
                    if ( i < 0 || j < 0 || i >= data.width || j >= data.height ) continue;
                    int v = bitmap->buffer[q * bitmap->width + p];
                    float t = v/255.0;
                    int k = i+(data.height-j-1)*data.width;
                    Color4f c = fg*t + convrt(data.image[k])*(1.0-t);
                    data.image[k] = convrt(c);
                }
            }
        }

        void draw_padding() {
            Color4f c = Color4f(1,0,1,1);
            int p = style.padding;
            for (int i = 0; i < data.height; i++ ) {
                for (int j = 0; j < data.width; j++ ) {
                    if (j > p && j < data.width - p && i > p && i < data.height - p) continue;

                    int k = j+(data.height-i-1)*data.width;
                    data.image[k] = convrt(c);
                }
            }
        }
};





VRText* VRText::get() {
    static VRText* singleton_opt = new VRText();
    return singleton_opt;
}

VRTexturePtr VRText::create(string text, string font, int res, int pad, Color4f fg, Color4f bg, int oline, Color4f oc, int spread) {
    style.ptSize = res;
    style.padding = pad;
    style.outline = oline;
    style.charspread = spread;
    return createBmp(text, font, fg, bg, oc);
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

/*void VRText::computeTexParams() {
    texWidth = resolution*maxLineLength*1.5;
    texHeight = resolution*1.5;
    texWidth += 2*padding;
    texHeight += 2*padding;
    texHeight *= Nlines;

    texWidth = osgNextPower2(texWidth);
    texHeight = osgNextPower2(texHeight);
}*/

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

VRTexturePtr VRText::createBmp(string text, string font, Color4f fg, Color4f bg, Color4f oc) {
    this->text = text;
    analyzeText();
    //computeTexParams();

    style.font = font;
    style.foreground = fg;
    style.background = bg;
    style.outlineColor = oc;

    FTRenderer ft;
    ft.style = style;
    ft.render(text);
    VRTexturePtr tex = VRTexture::create();
    tex->getImage()->set( Image::OSG_RGBA_PF, ft.data.width, ft.data.height, 1, 1, 1, 0, (UInt8*)&ft.data.image[0]);

    layoutWidth = ft.data.width - 2*ft.style.padding - 2*ft.style.outline;
    layoutHeight = ft.data.height - 2*ft.style.padding - 2*ft.style.outline;
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




