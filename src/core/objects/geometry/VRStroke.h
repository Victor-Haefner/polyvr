#ifndef VRStroke_H_INCLUDED
#define VRStroke_H_INCLUDED

#include "VRGeometry.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRStroke : public VRGeometry {
    public:
        enum CAP {
            NONE,
            ROUND,
            ARROW
        };

    private:
        vector<PathPtr> paths;
        vector<VRPolygonPtr> polygons;

        VRGeoDataPtr data;
        int mode = -1;
        vector<Vec3d> profile;
        bool closed = false;
        bool doColor = true;
        bool lit = true;
        bool doCaps = true;

        CAP cap_beg = NONE;
        CAP cap_end = NONE;

        VRGeometryPtr strewGeo = 0;

    public:
        VRStroke(string name);

        static VRStrokePtr create(string name = "None");
        VRStrokePtr ptr();

        void clear();

        void setPath(PathPtr p);
        void addPath(PathPtr p);
        PathPtr getPath();
        void setPaths(vector<PathPtr> p);
        vector<PathPtr> getPaths();

        void addPolygon(VRPolygonPtr p);

        void strokeProfile(vector<Vec3d> profile, bool closed, bool lit, bool doColor = true, CAP l = NONE, CAP r = NONE, bool doCaps = true);
        void strokeStrew(VRGeometryPtr geo);

        vector<Vec3d> getProfile();
        vector<Vec3d> makeRectProfile(double width, double height);
        vector<Vec3d> makeCircleProfile(double radius, int Npoints);

        void setDoColor(bool b);
        void convertToRope();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRStroke_H_INCLUDED
