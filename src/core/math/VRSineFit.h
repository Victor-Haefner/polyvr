#ifndef VRSINEFIT_H_INCLUDED
#define VRSINEFIT_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class SineFit {
    public:
        struct Fit {
            vector<double> guess;
            vector<double> fit; // A, w, p, c -> A sin ( w t + p ) +c
            double quality = 0;
        };

    public:
        vector<Vec2d> vertices;
        vector<Fit> fits;

    public:
        SineFit();
        ~SineFit();
        static SineFitPtr create();

        void setFFTFreqHint(int h, int s);

        void compute(int fftFreqHint);

        vector<double> getFrequencies();
        vector<double> getSineGuess(size_t sf = 0);
        vector<double> getSineApprox(size_t sf = 0);
};

OSG_END_NAMESPACE;

#endif // VRSINEFIT_H_INCLUDED
