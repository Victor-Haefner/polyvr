#ifndef INTERPOLATOR_H_INCLUDED
#define INTERPOLATOR_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>

using namespace std;

namespace OSG { class GeoVectorProperty; }

class interpolator {
    private:
        vector<OSG::Vec3d> pnts;
        vector<OSG::Vec3d> vals;

    public:
        interpolator();

        void setPoints(vector<OSG::Vec3d> pnts);
        void setValues(vector<OSG::Vec3d> vals);
        OSG::Vec3d eval(OSG::Vec3d& p, int power);
        void evalVec(OSG::GeoVectorProperty* pin, int power, OSG::GeoVectorProperty* cvec = 0, float cscale = 0, float dl_max = 1.0);
        void evalVec(vector<OSG::Vec3d>& pin, int power);
};

#endif // INTERPOLATOR_H_INCLUDED
