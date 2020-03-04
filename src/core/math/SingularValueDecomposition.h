#ifndef SINGULARVALUEDECOMPOSITION_H_INCLUDED
#define SINGULARVALUEDECOMPOSITION_H_INCLUDED

#include <vector>
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGMatrix.h>

using namespace std;

struct SingularValueDecomposition {
    OSG::Matrix4d U,S,V;
    int Nzeros = 0;
    bool success = false;

    SingularValueDecomposition(OSG::Matrix4d m, bool verbose = false);

    OSG::Matrix4d check();

    static void test();
};

#endif // SINGULARVALUEDECOMPOSITION_H_INCLUDED
