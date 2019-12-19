#ifndef TEXT2PNG_H_INCLUDED
#define TEXT2PNG_H_INCLUDED

#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRText {
    public:
        string text;
        size_t Nlines = 0;
        size_t maxLineLength = 0;

        int resolution = 20;
        int padding = 3;
        int texWidth = 1;
        int texHeight = 1;
        int layoutWidth = 0;
        int layoutHeight = 0;

        void analyzeText();
        void computeTexParams();
        void convertData(UChar8* data, int width, int height);
        VRTexturePtr createBmp (string text, string font, Color4f c, Color4f bg);

    public:
        static VRText* get();
        VRTexturePtr create(string text, string font, int res, Color4f fg, Color4f bg);

        static size_t countGraphemes(string txt);
        static vector<string> splitGraphemes(string txt);
};


OSG_END_NAMESPACE;


#endif // TEXT2PNG_H_INCLUDED
