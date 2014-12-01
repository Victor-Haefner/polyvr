#ifndef INTERPOLATOR_H_INCLUDED
#define INTERPOLATOR_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>

using namespace std;

class interpolator {
    private:
        vector<OSG::Vec3f> pnts;
        vector<OSG::Vec3f> vals;

    public:
        interpolator();

        void setPoints(vector<OSG::Vec3f> pnts);
        void setValues(vector<OSG::Vec3f> vals);
        void eval(OSG::Vec3f& p, int power);
        void evalVec(vector<OSG::Vec3f>& pin, int power);
};

#endif // INTERPOLATOR_H_INCLUDED
