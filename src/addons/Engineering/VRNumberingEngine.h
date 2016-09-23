#ifndef VRNUMBERINGENGINE_H_INCLUDED
#define VRNUMBERINGENGINE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

class VRNumberingEngine : public OSG::VRGeometry {
    private:
        OSG::VRGeoDataPtr data = 0;
        OSG::VRMaterialPtr mat = 0;

        static string vp;
        static string fp;
        static string gp;

        struct group {
            string pre, post;
        };

        vector<group> groups;

        void updateTexture();
        bool checkUIn(int grp);
        bool checkUIn(int i, int grp);

    public:
        VRNumberingEngine();

        static VRNumberingEnginePtr create();
        VRNumberingEnginePtr ptr();

        void clear();

        void add(OSG::Vec3f p = OSG::Vec3f(), int N = 1, float f = 0, int d = 2, int grp = 0);
        void set(int i, OSG::Vec3f p, float f = 0, int d = 2, int grp = 0);

        void setPrePost(int grp, string pre, string post);
        int addPrePost(string pre, string post);

        void setSize(float f);
        void setBillboard(bool b);
        void setOnTop(bool b);
};

#endif // VRNUMBERINGENGINE_H_INCLUDED
