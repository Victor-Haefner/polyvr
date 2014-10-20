#ifndef TEXT2PNG_H_INCLUDED
#define TEXT2PNG_H_INCLUDED

#include <OpenSG/OSGSimpleTexturedMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRText {

    private:

        void convertData(UChar8* data, int width, int height);

        ImageRecPtr createBmp (string text, string font, int width, int height, Color4f c, Color4f bg);

        //void setImage(SimpleTexturedMaterialRecPtr tex, ImageRecPtr img);

        //void write_png();

        VRText () { ; }
        void operator= (VRText v) {;}

    public:

        static VRText* get();

        ImageRecPtr create(string text, string font, int scale, Color4f fg, Color4f bg);

        SimpleTexturedMaterialRecPtr getTexture (string text, string font, int width, int height, Color4f fg, Color4f bg);
};


OSG_END_NAMESPACE;


#endif // TEXT2PNG_H_INCLUDED
