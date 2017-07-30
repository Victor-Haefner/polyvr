#ifndef OSGPANEL_H_INCLUDED
#define OSGPANEL_H_INCLUDED

#include "VRGeometry.h"
#include <OpenSG/OSGColor.h>

class CEF;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSprite : public VRGeometry {
    protected:
        string font = "SANS 20";
        Color4f fontColor;
        Color4f backColor;
        float width;
        float height;
        string label;
        shared_ptr<CEF> web;

        void updateGeo();

    public:
        VRSprite (string name, bool alpha = true, float w = 0.5, float h = 0.5);
        ~VRSprite();

        static VRSpritePtr create(string name = "None", bool alpha = true, float w = 0.5, float h = 0.5);
        VRSpritePtr ptr();

        void setSize(float w, float h);
        void setLabel(string l, float res = 1);
        void setTexture(string path);
        void webOpen(string path, int res, float ratio);
        void setFont(string f);
        void setFontColor(Color4f c);
        void setBackColor(Color4f c);

        Vec2d getSize();
        string getLabel();

        void convertToCloth();
};

OSG_END_NAMESPACE;

#endif // OSGPANEL_H_INCLUDED
