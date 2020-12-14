#ifndef VRMACHININGKINEMATICS_H_INCLUDED
#define VRMACHININGKINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "../VREngineeringFwd.h"
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMachiningKinematics : public std::enable_shared_from_this<VRMachiningKinematics> {
	private:
	public:
		VRMachiningKinematics();
		~VRMachiningKinematics();

		VRMachiningKinematicsPtr ptr();

		virtual void setEndEffector(PosePtr pose);
};

OSG_END_NAMESPACE;

#endif //VRMACHININGKINEMATICS_H_INCLUDED
