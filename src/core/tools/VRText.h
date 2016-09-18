#ifndef TEXT2PNG_H_INCLUDED
#define TEXT2PNG_H_INCLUDED

#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRText {
    private:
        void convertData(UChar8* data, int width, int height);
        VRTexturePtr createBmp (string text, string font, int width, int height, Color4f c, Color4f bg);

    public:
        static VRText* get();
        VRTexturePtr create(string text, string font, int height, Color4f fg, Color4f bg);
        VRTexturePtr create(string text, string font, int width, int height, Color4f fg, Color4f bg);
};


OSG_END_NAMESPACE;


#endif // TEXT2PNG_H_INCLUDED
