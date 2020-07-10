#ifndef OSGPANEL_H_INCLUDED
#define OSGPANEL_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include <OpenSG/OSGColor.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class CEF;

class VRSprite : public VRGeometry {
    protected:
        float width;
        float height;
        string label;
        VRGeoPrimitivePtr resizeTool;
        shared_ptr<CEF> web;
        bool usingShaders = false;

        static string spriteShaderVP;
        static string spriteShaderFP;

        void updateGeo();

    public:
        VRSprite (string name, bool alpha = true, float w = 0.5, float h = 0.5);
        ~VRSprite();

        static VRSpritePtr create(string name = "None", bool alpha = true, float w = 0.5, float h = 0.5);
        VRSpritePtr ptr();

        void setSize(float w, float h);
        VRTexturePtr setText(string l, float res = 1, Color4f c1 = Color4f(0,0,0,1), Color4f c2 = Color4f(0,0,0,0), int ol = 2, Color4f oc = Color4f(1,1,1,1), string font = "Mono.ttf");
        void setTexture(string path);
        void setBillboard(int i);
        void setScreensize(bool b);
        void webOpen(string path, int res, float ratio);

        Vec2d getSize();
        string getLabel();

        void showResizeTool(bool b, float size = 0.1, bool doAnnotations = true);
        void convertToCloth();
};

OSG_END_NAMESPACE;

#endif // OSGPANEL_H_INCLUDED
