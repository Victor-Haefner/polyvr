#ifndef VRANNOTATIONENGINE_H_INCLUDED
#define VRANNOTATIONENGINE_H_INCLUDED

#include "core/tools/VRToolsFwd.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

class VRAnnotationEngine : public VRGeometry {
    private:
        VRGeoDataPtr data = 0;
        VRMaterialPtr mat = 0;
        Vec4f fg, bg;

        static string vp;
        static string fp;
        static string gp;

        float size;

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

        static VRAnnotationEnginePtr create();
        VRAnnotationEnginePtr ptr();

        void clear();
        void set(int i, Vec3f p, string s);
        int add(Vec3f p, string s);

        void setSize(float f);
        void setColor(Vec4f c);
        void setBackground(Vec4f c);
        void setBillboard(bool b);
        void setScreensize(bool b);
};

OSG_END_NAMESPACE;

#endif // VRANNOTATIONENGINE_H_INCLUDED
