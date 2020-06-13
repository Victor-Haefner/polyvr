#ifndef VRANNOTATIONENGINE_H_INCLUDED
#define VRANNOTATIONENGINE_H_INCLUDED

#include <OpenSG/OSGColor.h>
#include "core/tools/VRToolsFwd.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

class VRAnnotationEngine : public VRGeometry {
    private:
        VRGeoDataPtr data = 0;
        VRMaterialPtr mat = 0;
        Color4f fg = Color4f(0,0,0,1);
        Color4f bg = Color4f(1,1,1,0);
        Color4f oc = Color4f(1,1,1,1);

        Vec3d orientationUp;
        Vec3d orientationDir;

        static string vp;
        static string fp;
        static string dfp;
        static string gp;

        static string vp_es2;
        static string fp_es2;

        float size;
        float oradius;
        float charTexSize;
        float texPadding;
        map<string, int> characterIDs;

        struct Label {
            Vec3d pos;
            vector<int> entries;
        };

        vector<Label> labels;

        void resize(Label& l, Vec3d p, int N);
        void updateTexture();
        bool checkUIn(int i);

    public:
        VRAnnotationEngine(string name);

        static VRAnnotationEnginePtr create(string name = "AnnotationEngine");
        VRAnnotationEnginePtr ptr();

        void clear();
        void set(int i, Vec3d p, string s);
        int add(Vec3d p, string s);

        void setSize(float f);
        void setColor(Color4f c);
        void setBackground(Color4f c);
        void setOutline(int r, Color4f c);
        void setBillboard(bool b);
        void setScreensize(bool b);
        void setOrientation(Vec3d d, Vec3d u);
};

OSG_END_NAMESPACE;

#endif // VRANNOTATIONENGINE_H_INCLUDED
