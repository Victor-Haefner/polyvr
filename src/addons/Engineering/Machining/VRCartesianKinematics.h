#ifndef VRCARTESIANKINEMATICS_H_INCLUDED
#define VRCARTESIANKINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "VRMachiningKinematics.h"
#include "../VREngineeringFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCartesianKinematics : public VRMachiningKinematics {
	private:
		VRTransformPtr axisX;
		VRTransformPtr axisY;
		VRTransformPtr axisZ;

	public:
		VRCartesianKinematics();
		~VRCartesianKinematics();

		static VRCartesianKinematicsPtr create();
		VRCartesianKinematicsPtr ptr();

		void setComponents(VRTransformPtr aX, VRTransformPtr aY, VRTransformPtr aZ);

		void setEndEffector(PosePtr pose);
};

OSG_END_NAMESPACE;

#endif //VRCARTESIANKINEMATICS_H_INCLUDED
