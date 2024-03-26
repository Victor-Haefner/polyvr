#include "PID.h"
#include "core/utils/VRTimer.h"

#include <iostream>

using namespace OSG;
using namespace std;

PID::PID() { time = VRTimer::create(); }
PID::~PID() {}

PIDPtr PID::create() { return PIDPtr( new PID() ); }
PIDPtr PID::ptr() { return static_pointer_cast<PID>(shared_from_this()); }

double PID::getIntegral() { return Int; }

void PID::clamp(double& v, const double& a, const double &b) {
    if (a > b) return;
    if (v > b) v = b;
    if (v < a) v = a;
}

void PID::setBounds(double a, double b, double mR) { min = a; max = b; maxRate = mR; }
void PID::setIntegralBounds(double a, double b, double Kw) { imin = a; imax = b; Kwin = Kw; }
void PID::setParameters(double Ke, double Kd, double Ki) { Kerr = Ke; Kder = Kd; Kint = Ki; }

void PID::setAutotune(bool run) { atRun = run; }

double PID::compute( double setpoint, double pv ) {
    double dt = time->stop();
    time->reset();
    if (dt <= 1e-9) return 0;

    if (atRun) { // TODO
        //Kp =
    }

    double e = setpoint - pv;
    double de = e-ePrev;

    // integral term
    double Dinc = incrSatPrev - incrPrev;
    Int += Kint * e * dt + Kwin * Dinc * dt;
    clamp(Int, imin, imax);

    // derivative term
    double d = (de + dtPrev * dPrev) / (dt + dtPrev);

    ePrev = e;
    dPrev = d;
    dtPrev = dt;

    // total increment
    double res = Kerr*e + Int + Kder*d;

    //cout << "PID compute " << Kerr << ", " << Kder << ", " << Kint << ",   " << e << ", " << d << ", " << Int << endl;

    incrPrev = res;
    clamp(res, min, max);
    if (maxRate > 0) clamp(res, incrSatPrev - maxRate*dt, incrSatPrev + maxRate*dt);
    incrSatPrev = res;
    return res;
}
