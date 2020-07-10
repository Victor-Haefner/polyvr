#ifndef TEXT2PNG_H_INCLUDED
#define TEXT2PNG_H_INCLUDED

#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct FontStyle {
    string font = "Mono.ttf";
    int ptSize = 16; // size in pixel
    int padding = 3; // padding in pixel
    int charspread = 0;
    int outline = 3; // outline in pixel
    int dpi = 100; // dpi, 100 means that ptSize is in pixel!

    Color4f foreground = Color4f(1,1,1,1);
    Color4f background = Color4f(1,1,1,0);
    Color4f outlineColor = Color4f(0,0,0,1);
};

struct TextLayout {
    int height = 0;
    int width = 0;
    int lineOffset = 0;
    vector<Vec4ub> image;
};

class VRText {
    public:
        string text;
        size_t Nlines = 0;
        size_t maxLineLength = 0;
        FontStyle style;

        int layoutWidth = 0;
        int layoutHeight = 0;

        void analyzeText();
        //void computeTexParams();
        void convertData(UChar8* data, int width, int height);
        VRTexturePtr createBmp (string text, string font, Color4f c, Color4f bg, Color4f oc);

    public:
        static VRText* get();
        VRTexturePtr create(string text, string font, int res, int padding, Color4f fg, Color4f bg, int outline = 2, Color4f oc = Color4f(1,1,1,1), int spread = 0);

        static size_t countGraphemes(string txt);
        static vector<string> splitGraphemes(string txt);
};


OSG_END_NAMESPACE;


#endif // TEXT2PNG_H_INCLUDED
