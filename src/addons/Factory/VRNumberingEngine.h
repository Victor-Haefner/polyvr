#ifndef VRNUMBERINGENGINE_H_INCLUDED
#define VRNUMBERINGENGINE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

class VRNumberingEngine : public OSG::VRGeometry {
    private:
        OSG::GeoVectorProperty* pos = 0;
        OSG::GeoVectorProperty* norms = 0; // n[0] is the number, n[1] is the ID

        static string vp;
        static string fp;
        static string gp;

        struct group {
            string pre, post;
        };

        vector<group> groups;
        OSG::VRMaterial* mat = 0;

        void updateTexture();
        bool checkUIn(int grp);
        bool checkUIn(int i, int grp);

    public:
        VRNumberingEngine();

        void add(OSG::Vec3f p = OSG::Vec3f(), int N = 1, float f = 0, int d = 2, int grp = 0);
        void set(int i, OSG::Vec3f p, float f = 0, int d = 2, int grp = 0);

        void setPrePost(int grp, string pre, string post);
        int addPrePost(string pre, string post);
};

#endif // VRNUMBERINGENGINE_H_INCLUDED
