#include "interpolator.h"
#include <OpenSG/OSGGeoVectorProperty.h>

using namespace OSG;

interpolator::interpolator() {}

void interpolator::setPoints(vector<Vec3f> pnts) { this->pnts = pnts; }
void interpolator::setValues(vector<Vec3f> vals) { this->vals = vals; }

Vec3f interpolator::eval(Vec3f& p, int power) { // frame
    Vec3f d;
    float Sw = 0, w = 0;
    for (int i=0; i<pnts.size(); i++) {
        if (i >= vals.size()) break;

        w = (p - pnts[i]).squareLength();
        w = 1.0/pow(w,power);
        Sw += w;
        d += vals[i]*w;
    }

    d *= 1.0/Sw;
    return d;
}

void interpolator::evalVec(GeoVectorProperty* pvec, int power, GeoVectorProperty* cvec, float cscale) {
    Vec3f* data = (Vec3f*)pvec->editData();
    Vec4f* cdata = 0;
    if (cvec) cdata = (Vec4f*)cvec->editData();
    float eps = 1e-5;
    for (int i=0; i<pvec->size(); i++) {
        Vec3f d = eval(data[i], power);
        data[i] += d;
        if (cdata) {
            float l = d.length() / max(cscale, eps);
            cdata[i] = Vec4f(l,1-l,0,1);
        }
    }
}

void interpolator::evalVec(vector<Vec3f>& pvec, int power) {
    for (Vec3f& p : pvec) eval(p, power);
}
