#ifndef VRStroke_H_INCLUDED
#define VRStroke_H_INCLUDED

#include "VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class path;

class VRStroke : public VRGeometry {
    private:
        vector<path*> paths;

        int mode = 0;
        vector<Vec3f> profile;
        bool closed = true;
        bool lit = true;

        VRGeometry* strewGeo = 0;

    public:
        VRStroke(string name);

        void setPath(path* p);
        void addPath(path* p);
        void setPaths(vector<path*> p);

        vector<path*>& getPaths();

        void strokeProfile(vector<Vec3f> profile, bool closed, bool lit);
        void strokeStrew(VRGeometry* geo);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRStroke_H_INCLUDED
