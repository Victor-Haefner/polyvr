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
        double Kerr = 0.1;
        double Kder = 0.1;
        double Kint = 0.01;
        double Err = 0;
        double Int = 0;

	public:
		PID();
		~PID();

		static PIDPtr create();
		PIDPtr ptr();

		double compute(double setpoint, double measurement);

		void setBounds(double min, double max);
		void setParameters(double Ke, double Kd, double Ki);
};

OSG_END_NAMESPACE;

#endif //VRPID_H_INCLUDED
