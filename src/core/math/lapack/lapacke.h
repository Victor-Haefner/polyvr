#ifndef _LAPACKE_H_
#define _LAPACKE_H_

#define LAPACK_ROW_MAJOR               101
#define LAPACK_COL_MAJOR               102

extern "C" {
    int dgeev_(int matrix_layout, char jobvl, char jobvr,
        int n, double* a, int lda, double* wr,
        double* wi, double* vl, int ldvl, double* vr,
        int ldvr);

    int dgesvd_(int matrix_layout, char jobu, char jobvt,
        int m, int n, double* a,
        int lda, double* s, double* u, int ldu,
        double* vt, int ldvt, double* superb);
}

#define dgeev dgeev_
#define dgesvd dgesvd_

#endif // _LAPACKE_H_