#ifndef OSGPANEL_H_INCLUDED
#define OSGPANEL_H_INCLUDED

#include "VRGeometry.h"
#include <OpenSG/OSGSimpleTexturedMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSprite : public VRGeometry {
    protected:
        string font;
        Color4f fontColor;
        float width;
        float height;
        string label;

    public:
        VRSprite (string name, bool alpha = true, float w = 0.5, float h = 0.5);
        ~VRSprite();

        void setSize(float w, float h);
        void setLabel(string l, float res = 1);
        void setTexture(string path);
        void webOpen(string path, int res, float ratio);
        void setFont(string f);
        void setFontColor(Color4f c);

        Vec2f getSize();
        string getLabel();
};

OSG_END_NAMESPACE;

#endif // OSGPANEL_H_INCLUDED
