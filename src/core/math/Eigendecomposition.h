#ifndef EIGENDECOMPOSITION_H_INCLUDED
#define EIGENDECOMPOSITION_H_INCLUDED

#include <vector>
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGMatrix.h>

using namespace std;

struct Eigendecomposition {
    vector<float> eigenvalues;
    vector<OSG::Vec4d> eigenvectors;
    int Nzeros = 0;
    double wr[3], wi[3], vl[9], vr[9];
    OSG::Vec3i reordering;
    bool success = false;

    void finalize(OSG::Vec3i order);

    Eigendecomposition(OSG::Matrix4d m, bool verbose = false);
};

#endif // EIGENDECOMPOSITION_H_INCLUDED
