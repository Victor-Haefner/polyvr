#include "Eigendecomposition.h"

#ifdef _WIN32
#include "core/math/lapack/lapacke.h"
#else
#include <lapacke.h>
#define dgeev LAPACKE_dgeev_work
#endif


using namespace OSG;

Eigendecomposition::Eigendecomposition(Matrix4d m, bool verbose) {
    int n = 3, lda = 3, ldvl = 3, ldvr = 3, info, lwork;
    double wkopt;
    double* work;
    double a[9] = { m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2] };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR
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

    if ( info > 0 ) { cout << "Warning: computeEigenvalues failed!\n"; return; } // Check for convergence

    Vec3i o(0,1,2);
    /*if (abs(wr[o[0]]) < abs(wr[o[1]])) swap(o[0], o[1]);
    if (abs(wr[o[1]]) < abs(wr[o[2]])) swap(o[1], o[2]);
    if (abs(wr[o[0]]) < abs(wr[o[1]])) swap(o[0], o[1]);*/
    if (wr[o[0]] < wr[o[1]]) swap(o[0], o[1]);
    if (wr[o[1]] < wr[o[2]]) swap(o[1], o[2]);
    if (wr[o[0]] < wr[o[1]]) swap(o[0], o[1]);
    if (verbose) {
        cout << "Eigendecomposition::Eigendecomposition: " << wr[0] << " " << wr[1] << " " << wr[2] << "   " << o << "    " << wr[o[0]] << " " << wr[o[1]] << " " << wr[o[2]] << " " << endl;
        cout << " M\n" << m << endl;
    }
    reordering = o;

    finalize(o);
    success = true;
    for (auto e : wr) if (abs(e) < 1e-6) Nzeros++;
}

void Eigendecomposition::finalize(Vec3i o) {
    eigenvalues.clear();
    eigenvectors.clear();
    eigenvalues.push_back(wr[o[0]]);
    eigenvalues.push_back(wr[o[1]]);
    eigenvalues.push_back(wr[o[2]]);
    eigenvectors.push_back(Vec4d(vl[o[0]*3], vl[o[0]*3+1], vl[o[0]*3+2], 0));
    eigenvectors.push_back(Vec4d(vl[o[1]*3], vl[o[1]*3+1], vl[o[1]*3+2], 0));
    eigenvectors.push_back(Vec4d(vl[o[2]*3], vl[o[2]*3+1], vl[o[2]*3+2], 0));
}

