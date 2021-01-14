#include "fft.h"


#ifndef WITHOUT_FFTW
#include <fftw3.h>
#endif

FFT::FFT() {}
FFT::~FFT() {}

vector<double> FFT::transform(vector<double>& F, size_t N) {
    vector<double> out(N);
#ifndef WITHOUT_FFTW
    fftw_plan ifft = fftw_plan_r2r_1d(N, &F[0], &out[0], FFTW_DHT, FFTW_ESTIMATE);
    fftw_execute(ifft);
    fftw_destroy_plan(ifft);
#endif
    return out;
}

vector<double> FFT::freq(size_t N, double d) {
    int n = N*0.5;
    if (N%2 == 1) n = (N-1)*0.5;
    double k = N*d;

    vector<double> r(2*n);
    for (int i=0; i<n; i++) {
        r[i] = i/k;
        r[2*n-i-1] = -(i+1)/k;
    }
    return r;
}
