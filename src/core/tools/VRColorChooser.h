#ifndef VRCOLORCHOOSER_H_INCLUDED
#define VRCOLORCHOOSER_H_INCLUDED

#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;

class VRColorChooser {
    private:
        float border = 0.05;
        Color3f color;
        Color3f last_color;
        VRGeometryPtr geo;
        VRMaterialPtr mat;

        Color3f colFromUV(Vec2f tc);
        void updateTexture();

    public:
        VRColorChooser();

        void setColor(Color3f c);
        Color3f getColor();
        Color3f getLastColor();

        void setGeometry(VRGeometryPtr geo);
        void resolve(VRDevice* dev);
};

OSG_END_NAMESPACE;

#endif // VRCOLORCHOOSER_H_INCLUDED
