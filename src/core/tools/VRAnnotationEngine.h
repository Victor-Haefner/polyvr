#ifndef VRANNOTATIONENGINE_H_INCLUDED
#define VRANNOTATIONENGINE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

class VRAnnotationEngine : public VRGeometry {
    private:
        GeoVectorProperty* pos = 0;
        GeoVectorProperty* norms = 0; // n[0] is the number, n[1] is the ID
        VRMaterial* mat = 0;
        Vec4f fg, bg;

        static string vp;
        static string fp;
        static string gp;

        struct Label {
            Vec3f pos;
            vector<int> entries;
        };

        vector<Label> labels;

        void resize(Label& l, Vec3f p, int N);
        void updateTexture();
        bool checkUIn(int i);

    public:
        VRAnnotationEngine();

        void clear();

        void set(int i, Vec3f p, string s);

        void setSize(float f);
        void setColor(Vec4f c);
        void setBackground(Vec4f c);
        void setBillboard(bool b);
        void setOnTop(bool b);
};

OSG_END_NAMESPACE;

#endif // VRANNOTATIONENGINE_H_INCLUDED
