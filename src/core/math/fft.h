#ifndef FFT_H_INCLUDED
#define FFT_H_INCLUDED

#include <vector>

using namespace std;

class FFT {
    private:
    public:
        FFT();
        ~FFT();

        vector<double> freq(size_t N, double d);
        vector<double> transform(vector<double>& F, size_t N);
};

#endif // FFT_H_INCLUDED
