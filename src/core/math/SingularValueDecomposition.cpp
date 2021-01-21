#include "SingularValueDecomposition.h"
#include "Eigendecomposition.h"

#ifdef _WIN32
#include "core/math/lapack/lapacke.h"
#else
#include <lapacke.h>
#define dgesvd LAPACKE_dgesvd_work
#endif


using namespace OSG;

SingularValueDecomposition::SingularValueDecomposition(Matrix4d H, bool verbose) {
    /*Matrix4d Ht, M;
    H.transposed(Ht);

    M = Ht;
    M.mult(H);
    Eigendecomposition eigenV(M, verbose);
    V[0] = eigenV.eigenvectors[0];
    V[1] = eigenV.eigenvectors[1];
    V[2] = eigenV.eigenvectors[2];

    M = H;
    M.mult(Ht);
    Eigendecomposition eigenU(M, verbose);
    U[0] = eigenU.eigenvectors[0];
    U[1] = eigenU.eigenvectors[1];
    U[2] = eigenU.eigenvectors[2];

    vector<float> evs;
    for (auto v : eigenU.eigenvalues) evs.push_back(sqrt(abs(v)));
    int N = 3;
    if (evs.size() < N) N = evs.size();
    for (int i=0; i<N; i++) {
        S[i][i] = evs[i];
    }

    Nzeros = eigenU.Nzeros;
    success = true;*/


    /* Locals */

    const int ml = LAPACK_ROW_MAJOR;
    int m = 3, n = 3, lda = 3, ldu = 3, ldv = 3;
    int info, lwork;
    double wkopt;
    double* work;
    /* Local arrays */
    double s[3], u[9], v[9];
    double a[9] = {
        H[0][0],  H[1][0], H[2][0],
        H[0][1],  H[1][1], H[2][1],
        H[0][2],  H[1][2], H[2][2]
    };

#ifdef _WIN32
    //double superb[2];
    //info = dgesvd(ml, 'a', 'a', m, n, a, lda, s, u, ldu, v, ldv, superb);
    lwork = -1;
    dgesvd("All", "All", &m, &n, a, &lda, s, u, &ldu, v, &ldv, &wkopt, &lwork, &info);
    lwork = (int)wkopt;
    work = new double[lwork];
    dgesvd("All", "All", &m, &n, a, &lda, s, u, &ldu, v, &ldv, work, &lwork, &info);
    delete work;
#else
    lwork = -1;
    info = dgesvd( ml, 'a', 'a', m, n, a, lda, s, u, ldu, v, ldv, &wkopt, lwork );
    lwork = (int)wkopt;
    work = (double*)malloc( lwork*sizeof(double) );
    info = dgesvd( ml, 'a', 'a', m, n, a, lda, s, u, ldu, v, ldv, work, lwork );
    free((void*)work);
#endif
    if( info > 0 ) { printf( "The algorithm computing SVD failed to converge.\n" ); }

    S.setScale(Vec3d(s[0], s[1], s[2]));
    U = Matrix4d(   u[0], u[1], u[2], 0,
                    u[3], u[4], u[5], 0,
                    u[6], u[7], u[8], 0,
                    0, 0, 0, 1);
    /*V = Matrix4d(   v[0], v[1], v[2], 0,
                    v[3], v[4], v[5], 0,
                    v[6], v[7], v[8], 0,
                    0, 0, 0, 1);*/
    V = Matrix4d(   v[0], v[3], v[6], 0,
                    v[1], v[4], v[7], 0,
                    v[2], v[5], v[8], 0,
                    0, 0, 0, 1);
}

Matrix4d SingularValueDecomposition::check() { // M = U S Vt
    Matrix4d USVt, Vt;
    Vt.transposeFrom(V);
    USVt = U;
    USVt.mult(S);
    USVt.mult(Vt);
    return USVt;
}

void SingularValueDecomposition::test() {
    Matrix4d M = Matrix4d(0,1,1,0, sqrt(2),2,0,0, 0,1,1,0, 0,0,0,1);
    Matrix4d P = Matrix4d(1/sqrt(6),-1/sqrt(3),1/sqrt(2),0, 2/sqrt(6),1/sqrt(3),0,0, 1/sqrt(6),-1/sqrt(3),-1/sqrt(2),0, 0,0,0,1);
    Matrix4d Q = Matrix4d(1/sqrt(6),1/sqrt(3),1/sqrt(2),0, 3/sqrt(12),0,-0.5,0, 1/sqrt(12),-2/sqrt(6),0.5,0, 0,0,0,1);
    SingularValueDecomposition svd(M);
    cout << "\nSVDtest\n";
    cout << "M\n" << M << endl;
    cout << "S\n" << svd.S << endl;
    cout << "U\n" << svd.U << endl;
    cout << "P\n" << P << endl;
    cout << "V\n" << svd.V << endl;
    cout << "Q\n" << Q << endl;
    Matrix4d R = svd.U;
    R.mult(svd.S);
    svd.V.transpose();
    R.mult(svd.V);
    cout << "R\n" << R << endl;

    Matrix4d B = P;
    B.mult(svd.S);
    Q.transpose();
    B.mult(Q);
    cout << "Qt\n" << Q << endl;
    cout << "B\n" << B << endl;
}
