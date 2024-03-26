#ifndef VRPID_H_INCLUDED
#define VRPID_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRMathFwd.h"
#include "core/utils/VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class PID : public std::enable_shared_from_this<PID> {
	private:
	    VRTimerPtr time;

        double min = 1;
        double max = -1;
        double imin = 1;
        double imax = -1;
        double Kerr = 0.1;
        double Kder = 0.1;
        double Kint = 0.01;
        double Kwin = 0.01;
        double maxRate = -1;
        double ePrev = 0;
        double dPrev = 0;
        double dtPrev = 0;
        double Int = 0;

        bool atRun;
        double incrPrev = 0;
        double incrSatPrev = 0;

        void clamp(double& v, const double& a, const double &b);

	public:
		PID();
		~PID();

		static PIDPtr create();
		PIDPtr ptr();

		double compute(double setpoint, double measurement);
		double getIntegral();

		void setBounds(double min, double max, double maxRate);
		void setIntegralBounds(double min, double max, double Kw);
		void setParameters(double Ke, double Kd, double Ki);

		void setAutotune(bool run);
};

OSG_END_NAMESPACE;

#endif //VRPID_H_INCLUDED
