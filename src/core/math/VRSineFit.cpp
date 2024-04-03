#include "VRSineFit.h"
#include "core/utils/toString.h"
#include "core/math/PCA.h"
#include "core/math/fft.h"

#include <unsupported/Eigen/NonLinearOptimization>

using namespace OSG;
using namespace Eigen;

SineFit::SineFit() {}
SineFit::~SineFit() {}

SineFitPtr SineFit::create() { return SineFitPtr(new SineFit()); }

double calcMean(vector<double>& f) {
    double m = 0;
    for (auto& v : f) m += v;
    m /= f.size();
    return m;
}

double calcMeanDeviation(vector<double>& f, double mean) {
    double S = 0;
    for (auto v : f) {
        S += abs(v-mean);
    }
    return S / f.size();
}

vector<int> getArgMax(const vector<double>& Fyy, const vector<double>& ff, int offset, int N) {
    vector<int> r(N,0);
    vector<double> m(N,-1e6);
    for (unsigned int i=offset; i<Fyy.size(); i++) {
        //if (ff[i] < 0) continue; // gabler demo needs that, ok for FT?
        for (int j=0; j<N; j++) {
            if (Fyy[i] > m[j]) {
                m[j] = Fyy[i];
                r[j] = i;
                break;
            }
        }
    }

    cout << "FFT results:";
    for (auto i=0; i<N; i++) {
        cout << " (" << r[i] << "," << m[i] << ")";
    }
    cout << endl;

    return r;
}

struct FunctorSine {
    typedef double Scalar;
    typedef Eigen::Matrix<double,Dynamic,1> InputType;
    typedef Eigen::Matrix<double,Dynamic,1> ValueType;
    typedef Eigen::Matrix<double,Dynamic,Dynamic> JacobianType;

    enum {
        InputsAtCompileTime = Dynamic,
        ValuesAtCompileTime = Dynamic
    };

    vector<double> X;
    vector<double> Y;
    int m_inputs = 4;
    int m_values = Dynamic;
    double delta = 1.0;

    FunctorSine(vector<double> x, vector<double> y) : X(x), Y(y),  m_values(X.size()) {}

    int inputs() const { return m_inputs; }
    int values() const { return m_values; }

    // the error to sine function
    int operator() (const VectorXd &p, VectorXd &f) const {
        double totalResiduals = 0.0;
        double totalLoss = 0.0;
        for (int i = 0; i < values(); i++) {
            if (Y[i] < 71.0) {
                f[i] = 0;
            }

            double sine = p[0] * sin(p[1]*X[i] + p[2]) + p[3];
            double residual = Y[i] - sine;
            f[i] = residual;
            //f[i] = huberLoss(residual, delta); // delta is a tuning parameter for Huber loss
            totalLoss += f[i];
            totalResiduals += residual;
            //cout << "  residual: " << Y[i] << " - " << sine << " = " << residual << endl;
        }

        //cout << "  FunctorSine " << p[0] << ", " << p[1] << ", " << p[2] << ", " << p[3] << " --> " << totalLoss << ", " << totalResiduals << endl;
        return 0;
    }

    // reduce impact of outliers
    double huberLoss(double residual, double delta) const {
        if (abs(residual) <= delta) return 0.5 * residual * residual;
        else return delta * (abs(residual) - 0.5 * delta);
    }

    /*
    a*x + d             -> a
    a sin(b x + c) + d  -> a cos(b x + c) b
    a sin(x + c) + d    -> a cos(b x + c)
    a*x + d             -> a
    */

    // jacoby matrices, contains the partial derivatives
    /*int df(const VectorXd &p, MatrixXd &jac) const {
        for (int i = 0; i < values(); i++) {
            jac(i,0) = -sin(p[1]*X[i] + p[2]);
            jac(i,1) = -p[0] * cos(p[1]*X[i] + p[2])*X[i];
            jac(i,2) = -p[0] * cos(p[1]*X[i] + p[2]);
            jac(i,3) = -1;
        }
        return 0;
    }*/
};

double CalculateSSR(const Eigen::VectorXd& observed, const Eigen::VectorXd& fitted) {
    Eigen::VectorXd residuals = observed - fitted;
    double ssr = residuals.squaredNorm();
    return ssr;
}

double CalculateRSquared(const Eigen::VectorXd& observed, const Eigen::VectorXd& fitted) {
    double ssr = CalculateSSR(observed, fitted);
    double m = observed.mean();
    vector<double> mv(observed.size(), m);
    Eigen::VectorXd meanv = Map<VectorXd>(&mv[0], mv.size());
    double sst = (observed - meanv).squaredNorm();
    double rsquared = 1.0 - (ssr / sst);
    return rsquared;
}

void SineFit::compute(int fftFreqHint) {
    //Fit sin to the input time sequence, and store fitting parameters "amp", "omega", "phase", "offset", "freq", "period" and "fitfunc")
    if (vertices.size() < 3) return;

    double pi = Pi;

    vector<double> X, Y;
    for (auto& p : vertices) {
        X.push_back(p[0]+pi);
        Y.push_back(p[1]);
    }

    double guess_offset = calcMean(Y);
    double guess_amp = calcMeanDeviation(Y, guess_offset) * sqrt(2);

    //TODO: check the fft, the result is pretty bad..
    double Df = X[X.size()-1] - X[0];
    Df = 2*pi*(2*pi/Df);

    FFT fft;
    vector<double> ff = fft.freq(X.size(), X[1]-X[0]);
    vector<double> Fyy = fft.transform(Y, Y.size());
    for (auto& f : Fyy) f = abs(f);

    //cout << " ff " << toString(ff) << endl;
    //cout << " yy " << toString(Fyy) << endl;

    /*enum Status {
        NotStarted = -2,
        Running = -1,
        ImproperInputParameters = 0,
        RelativeReductionTooSmall = 1,
        RelativeErrorTooSmall = 2,
        RelativeErrorAndReductionTooSmall = 3,
        CosinusTooSmall = 4,
        TooManyFunctionEvaluation = 5,
        FtolTooSmall = 6,
        XtolTooSmall = 7,
        GtolTooSmall = 8,
        UserAsked = 9
    };*/

    vector<int> freqs = getArgMax(Fyy, ff, 1, fftFreqHint); // excluding the zero frequency "peak", which is related to offset
    for (auto FyyMaxPos : freqs) {
        //double guess_freq = abs(ff[FyyMaxPos]);
        double guess_freq = ff[FyyMaxPos];

        Fit sf;
        sf.guess = { guess_amp, Df*guess_freq, 0, guess_offset }; //
        VectorXd x = Map<VectorXd>(&sf.guess[0], sf.guess.size());
        //cout << "SineFit::compute guess, A: " << sf.guess[0] << " f: " << sf.guess[1] << ", ff: " << ff[FyyMaxPos] << ", Fyy: " << Fyy[FyyMaxPos] << endl;

        FunctorSine functor(X,Y);
        functor.delta = guess_amp*0.1;
        //LevenbergMarquardt<FunctorSine> lm(functor);
        NumericalDiff<FunctorSine> numDiff(functor);
        LevenbergMarquardt<NumericalDiff<FunctorSine>,double> lm(numDiff);

        lm.parameters.factor = 100; // 100 // step size in the optimization process
        lm.parameters.maxfev = 100*(x.size()+1); // 400  // max number of iterations
        lm.parameters.ftol = 1e-6; // eps
        lm.parameters.xtol = 1e-6; // eps
        lm.parameters.gtol = 0; // 0
        lm.parameters.epsfcn = 0; // 0
        int status = lm.minimize(x);
        sf.fit = vector<double>( x.data(), x.data() + 4 );
        sf.quality = lm.fnorm;


        //sf.fit[0] = sf.guess[0];
        //sf.fit[1] = sf.guess[1];

        //cout << " SineFit::compute fit, A: " << sf.fit[0] << ", f: " << sf.fit[1] << ", qual: " << sf.quality << ", status: " << status << ", max itr: " << lm.parameters.maxfev << endl;
        fits.push_back(sf);
    }
}

vector<double> SineFit::getFrequencies() {
    vector<double> res;
    for (auto sf : fits) res.push_back(sf.fit[1]);
    return res;
}

vector<double> SineFit::getSineGuess(size_t sf) {
    if (fits.size() <= sf) return vector<double>();
    return fits[sf].guess;
}

vector<double> SineFit::getSineApprox(size_t sf) {
    if (fits.size() <= sf) return vector<double>();
    return fits[sf].fit;
}





