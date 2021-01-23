#include "PCA.h"
#include "core/utils/toString.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"

#ifdef _WIN32
#include "core/math/lapack/lapacke.h"
#else
#include <lapacke.h>
#define dgeev LAPACKE_dgeev_work
#endif


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
    Matrix4d res(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0); // 3x3?


    int N = pnts.size();

    for (auto& pg : pnts) {
        Vec3d p = pg - center;
        res[0][0] += p[0]*p[0];
        res[1][1] += p[1]*p[1];
        res[2][2] += p[2]*p[2];
        res[0][1] += p[0]*p[1];
        res[0][2] += p[0]*p[2];
        res[1][2] += p[1]*p[2];
    }

    cout << "PCA::computeCovMatrix " << center << " " << N << endl;

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

void PCA::test() {
/*#ifdef _WIN32
    lapack_int n = 3, lda = 3, ldvl = 3, ldvr = 3;
    double wr[3], wi[3], vl[9], vr[9];
    double a[9] = { 0,1,2,3,4,5,6,7,8 };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR
    cout << " PCA::computeEigenvectors, run dgeev test on:" << endl << a[0] << " " << a[1] << " " << a[2] << " " << a[3] << " " << a[4] << " " << a[5] << " " << a[6] << " " << a[7] << " " << a[8] << endl;

    lapack_int info = dgeev(LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr);
    cout << "   dgeev test done" << endl;
#endif*/
}

Matrix4d PCA::computeEigenvectors(Matrix4d m) {
    //test();

    lapack_int n = 3, lda = 3, ldvl = 3, ldvr = 3;
    lapack_int info, lwork;
    double wkopt;
    double* work;
    double wr[3], wi[3], vl[9], vr[9];
    double a[9] = { m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2] };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR
    cout << " PCA::computeEigenvectors, run dgeev on:" << endl << m[0][0] <<" "<< m[1][0] << " " << m[2][0] << " " << m[0][1] << " " << m[1][1] << " " << m[2][1] << " " << m[0][2] << " " << m[1][2] << " " << m[2][2] << endl;
#ifdef _WIN32
    //info = dgeev(LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr);
    lwork = -1;
    dgeev("Vectors", "Vectors", &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, &wkopt, &lwork, &info);
    lwork = (int)wkopt;
    work = new double[lwork];
    dgeev("Vectors", "Vectors", &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, work, &lwork, &info);
    delete work;
#else
    lwork = -1;
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr, &wkopt, lwork);
    lwork = (int)wkopt;
    work = new double[lwork];
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr,  work, lwork);
    delete work;
#endif
    cout << "   dgeev done" << endl;

    if ( info > 0 ) { cout << "Warning: computeEigenvalues failed!\n"; return Matrix4d(); } // Check for convergence

    Vec3i o(0,1,2);
    if (wr[o[0]] < wr[o[1]]) swap(o[0], o[1]);
    if (wr[o[1]] < wr[o[2]]) swap(o[1], o[2]);
    if (wr[o[0]] < wr[o[1]]) swap(o[0], o[1]);
    cout << " pca reorder: " << o << ",  ew: " << Vec3d(wr[0], wr[1], wr[2]) << " -> " << Vec3d(wr[o[0]], wr[o[1]], wr[o[2]]) << endl;

    Matrix4d res;
    res[0] = Vec4d(vl[o[0]*3], vl[o[0]*3+1], vl[o[0]*3+2], 0);
    res[1] = Vec4d(vl[o[1]*3], vl[o[1]*3+1], vl[o[1]*3+2], 0);
    res[2] = Vec4d(vl[o[2]*3], vl[o[2]*3+1], vl[o[2]*3+2], 0);
    res[3] = Vec4d(wr[o[0]], wr[o[1]], wr[o[2]], 0);
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
void PCA::clear() { pnts.clear(); }

void PCA::addMesh(VRObjectPtr obj) {
    for (auto child : obj->getChildren(true, "Geometry", true)) {
        auto geo = dynamic_pointer_cast<VRGeometry>(child);
        Matrix4d M = geo->getMatrixTo(obj);
        M.invert();
        auto positions = geo->getMesh()->geo->getPositions();
        for (size_t i=0; i<positions->size(); i++) {
            Pnt3d p = Pnt3d(positions->getValue<Pnt3f>(i));
            M.mult(p,p);
            pnts.push_back(Vec3d(p));
        }
    }
}





