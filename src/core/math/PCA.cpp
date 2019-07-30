#include "PCA.h"
#include "core/utils/toString.h"

#include <lapacke.h>
#define dgeev LAPACKE_dgeev_work

using namespace OSG;

template<> string typeName(const PCA& t) { return "PCA"; }

Vec3d PCA::computeCentroid() {
    Vec3d res;
    int N = pnts.size();
    for (auto& p : pnts) res += p;
    if (N > 0) res *= 1.0/N;
    cout << " centroid: " << res << endl;
    return res;
}

Matrix4d PCA::computeCovMatrix() {
    Vec3d center = computeCentroid();
    int N = pnts.size();
    Matrix4d res(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0); // 3x3?

    for (auto& pg : pnts) {
        Vec3d p = pg - center;
        res[0][0] += p[0]*p[0];
        res[1][1] += p[1]*p[1];
        res[2][2] += p[2]*p[2];
        res[0][1] += p[0]*p[1];
        res[0][2] += p[0]*p[2];
        res[1][2] += p[1]*p[2];
    }

    cout << "VRSelection::computeCovMatrix " << center << " " << N << endl;

    for (int i=0; i<3; i++)
        for (int j=i; j<3; j++) res[i][j] *= 1.0/N;

    res[1][0] = res[0][1];
    res[2][0] = res[0][2];
    res[2][1] = res[1][2];

    res[3][0] = center[0];
    res[3][1] = center[1];
    res[3][2] = center[2];

    cout << " covariance matrix: " << res[0][0] << " " << res[1][1] << " " << res[2][2] << " " << res[0][1] << " " << res[0][2] << " " << res[1][2] << endl;
    return res;
}

Matrix4d PCA::computeEigenvectors(Matrix4d m) {
    int n = 3, lda = 3, ldvl = 3, ldvr = 3, info, lwork;
    double wkopt;
    double* work;
    double wr[3], wi[3], vl[9], vr[9];
    double a[9] = { m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2] };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR

    lwork = -1;
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr, &wkopt, lwork);
    lwork = (int)wkopt;
    work = new double[lwork];
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr,  work, lwork);
    delete work;

    if ( info > 0 ) { cout << "Warning: computeEigenvalues failed!\n"; return Matrix4d(); } // Check for convergence

    Vec3i o(0,1,2);
    if (wr[0] > wr[1]) swap(o[0], o[1]);
    if (wr[1] > wr[2]) swap(o[1], o[2]);
    if (wr[0] > wr[1]) swap(o[0], o[1]);

    Matrix4d res;
    res[0] = Vec4d(vl[o[2]*3], vl[o[2]*3+1], vl[o[2]*3+2], 0);
    res[1] = Vec4d(vl[o[1]*3], vl[o[1]*3+1], vl[o[1]*3+2], 0);
    res[2] = Vec4d(vl[o[0]*3], vl[o[0]*3+1], vl[o[0]*3+2], 0);
    res[3] = Vec4d(wr[o[2]], wr[o[1]], wr[o[0]], 0);
    return res;
}

Pose PCA::compute() { // TODO: ditch it at some point!
    Pose res;
    Matrix4d cov = computeCovMatrix();
    Matrix4d ev  = computeEigenvectors(cov);
    res.set(Vec3d(cov[3]), Vec3d(ev[0]), Vec3d(ev[2]), Vec3d(ev[3]));
    return res;
}

PCA::PCA() {}
PCA::~PCA() {}
PCAPtr PCA::create() { return PCAPtr( new PCA() ); }

void PCA::add(Vec3d p) { pnts.push_back(p); }
int PCA::size() { return pnts.size(); }






