#ifndef INTERPOLATOR_H_INCLUDED
#define INTERPOLATOR_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>

using namespace std;

namespace OSG { class GeoVectorProperty; }

class interpolator {
    private:
        vector<OSG::Vec3f> pnts;
        vector<OSG::Vec3f> vals;

    public:
        interpolator();

        void setPoints(vector<OSG::Vec3f> pnts);
        void setValues(vector<OSG::Vec3f> vals);
        OSG::Vec3f eval(OSG::Vec3f& p, int power);
        void evalVec(OSG::GeoVectorProperty* pin, int power, OSG::GeoVectorProperty* cvec = 0, float cscale = 0, float dl_max = 1.0);
        void evalVec(vector<OSG::Vec3f>& pin, int power);
};

#endif // INTERPOLATOR_H_INCLUDED
