#include "interpolator.h"
#include <OpenSG/OSGGeoVectorProperty.h>

using namespace OSG;

interpolator::interpolator() {}

void interpolator::setPoints(vector<Vec3d> pnts) { this->pnts = pnts; }
void interpolator::setValues(vector<Vec3d> vals) { this->vals = vals; }

Vec3d interpolator::eval(Vec3d& p, int power) { // frame
    Vec3d d;
    float Sw = 0, w = 0;
    for (unsigned int i=0; i<pnts.size(); i++) {
        if (i >= vals.size()) break;

        w = (p - pnts[i]).squareLength();
        w = 1.0/pow(w,power);
        Sw += w;
        d += vals[i]*w;
    }

    d *= 1.0/Sw;
    return d;
}

void interpolator::evalVec(GeoVectorProperty* pvec, int power, GeoVectorProperty* cvec, float cscale, float dl_max) {
    Vec3d* data = (Vec3d*)pvec->editData();
    Vec4d* cdata = 0;
    if (cvec) cdata = (Vec4d*)cvec->editData();
    float eps = 1e-5;
    float dl;
    for (unsigned int i=0; i<pvec->size(); i++) {
        Vec3d d = eval(data[i], power);
        data[i] += d;
        if (cdata) {
            dl = d.length();
            float l = dl / max(cscale*dl_max, eps);
            cdata[i] = Vec4d(l, 1-l, 0, 1);
        }
    }
}

void interpolator::evalVec(vector<Vec3d>& pvec, int power) {
    for (Vec3d& p : pvec) p += eval(p, power);
}
