#ifndef _LAPACKE_H_
#define _LAPACKE_H_

#define LAPACK_ROW_MAJOR               101
#define LAPACK_COL_MAJOR               102

#define lapack_int int

extern "C" {
    void dgeev_(char* jobvl, char* jobvr, lapack_int* n, double* a,
        lapack_int* lda, double* wr, double* wi, double* vl,
        lapack_int* ldvl, double* vr, lapack_int* ldvr, double* work,
        lapack_int* lwork, lapack_int* info);

    void dgesvd_(char* jobu, char* jobvt, lapack_int* m, lapack_int* n,
        double* a, lapack_int* lda, double* s, double* u,
        lapack_int* ldu, double* vt, lapack_int* ldvt, double* work,
        lapack_int* lwork, lapack_int* info);
}

#define dgeev dgeev_
#define dgesvd dgesvd_

#endif // _LAPACKE_H_
