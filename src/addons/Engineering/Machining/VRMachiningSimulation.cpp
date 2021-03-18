#include "VRMachiningSimulation.h"
#include "VRMachiningKinematics.h"
#include "VRMachiningCode.h"

#include "core/objects/VRAnimation.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRTimer.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"

using namespace OSG;

VRMachiningSimulation::VRMachiningSimulation() {}
VRMachiningSimulation::~VRMachiningSimulation() {}

VRMachiningSimulationPtr VRMachiningSimulation::create() { return VRMachiningSimulationPtr( new VRMachiningSimulation() ); }
VRMachiningSimulationPtr VRMachiningSimulation::ptr() { return static_pointer_cast<VRMachiningSimulation>(shared_from_this()); }

void VRMachiningSimulation::setKinematics(VRMachiningKinematicsPtr k) { kinematics = k;  }
void VRMachiningSimulation::setCode(VRMachiningCodePtr c) { code = c; }
void VRMachiningSimulation::setOnFinish(VRUpdateCbPtr cb) { finishCallback = cb; }

void VRMachiningSimulation::stop() { doStop = true; }

void VRMachiningSimulation::start(double sM) {
	cout << "VRMachiningSimulation::start " << sM << endl;
	if (!code) { cout << " Error, no code" << endl; return;  }
	if (!kinematics) { cout << " Error, no kinematics" << endl; return; }

	doStop = false;
	speedMultiplier = sM;
	timer.reset();
	code->reset();
	runInstruction();
}

void VRMachiningSimulation::runInstruction(float delay) {
	if (doStop) return;

	if (!code || code->length() == 0) {
		double T = timer.stop();
		run_time = T * speedMultiplier;
		cout << "runGCode finished after " << T << " seconds" << endl;
		if (finishCallback) (*finishCallback)();
		return;
	}

	VRMachiningCode::Instruction c = code->next();
	eeD = c.d;
	eeP0 = c.p0;

	anim = VRAnimation::create("MachiningAnim");
	anim->setCallback( VRAnimCb::create( "MachiningAnim", bind(&VRMachiningSimulation::run, this, std::placeholders::_1) ) );
	anim->setDuration(c.T);
	anim->start(delay);
}

void VRMachiningSimulation::run(float t) {
	if (doStop) return;
	Vec3d ee = eeP0 + eeD * t;
	kinematics->setEndEffector(Pose::create(ee));
	if (t == 1) runInstruction();
}
