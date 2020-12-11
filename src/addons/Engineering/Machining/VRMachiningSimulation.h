#ifndef VRMACHININGSIMULATION_H_INCLUDED
#define VRMACHININGSIMULATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include "../VREngineeringFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRTimer.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMachiningSimulation : public std::enable_shared_from_this<VRMachiningSimulation> {
	private:
		VRMachiningKinematicsPtr kinematics;
		VRMachiningCodePtr code;
		VRUpdateCbPtr finishCallback;

		bool doStop = false;
		double speedMultiplier = 1;
		double start_time = 0;
		double run_time = 0;
		Vec3d eeD;
		Vec3d eeP0;

		VRTimer timer;
		VRAnimationPtr anim;

		void run(float t);
		void runInstruction(float delay = 0);

	public:
		VRMachiningSimulation();
		~VRMachiningSimulation();

		static VRMachiningSimulationPtr create();
		VRMachiningSimulationPtr ptr();

		void setKinematics(VRMachiningKinematicsPtr kinematics);
		void setCode(VRMachiningCodePtr code);
		void setOnFinish(VRUpdateCbPtr cb);

		void start(double sM = 1.0);
		void stop();
};

OSG_END_NAMESPACE;

#endif // VRMACHININGSIMULATION_H_INCLUDED
