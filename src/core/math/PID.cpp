#include "PID.h"
#include "core/utils/VRTimer.h"

using namespace OSG;

PID::PID() { time = VRTimer::create(); }
PID::~PID() {}

PIDPtr PID::create() { return PIDPtr( new PID() ); }
PIDPtr PID::ptr() { return static_pointer_cast<PID>(shared_from_this()); }

void PID::setBounds(double a, double b) { min = a; max = b; }
void PID::setParameters(double Ke, double Kd, double Ki) { Kerr = Ke; Kder = Kd; Kint = Ki; }

double PID::compute( double setpoint, double pv ) {
    double dt = time->stop();
    time->reset();
    if (dt <= 1e-9) return 0;

    double e = setpoint - pv;
    double de = e-Err;
    Err = e;

    Int += e * dt; // integrate

    double d = de / dt; // derivative

    double res = Kerr*e + Kint*Int + Kder*d; // total increment

    if (max >= min) { // clamp res
        if ( res > max ) res = max;
        else if ( res < min ) res = min;
    }

    return res;
}
