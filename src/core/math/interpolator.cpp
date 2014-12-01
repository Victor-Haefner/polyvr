#include "interpolator.h"

using namespace OSG;

interpolator::interpolator() {}

void interpolator::setPoints(vector<Vec3f> pnts) { this->pnts = pnts; }
void interpolator::setValues(vector<Vec3f> vals) { this->vals = vals; }

void interpolator::eval(Vec3f& p, int power) { // frame
        Vec3f d;
        float Sw = 0, w = 0;
        for (int i=0; i<pnts.size(); i++) {
            if (i >= vals.size()) break;

            Vec3f pnt = pnts[i];
            Vec3f val = vals[i];

            w = (p-pnt).length();
            w = 1.0/pow(w,power);
            Sw += w;
            d += val*w;
        }

        d *= 1.0/Sw;
        p[0] = d[0];// TODO: optimize
        p[1] = d[1];
        p[2] = d[2];
}

void interpolator::evalVec(vector<Vec3f>& pvec, int power) {
    for (Vec3f& p : pvec) eval(p, power);
}
