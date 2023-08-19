#ifndef VRCARTESIANKINEMATICS_H_INCLUDED
#define VRCARTESIANKINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <vector>
#include <OpenSG/OSGVector.h>

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
		Vec3d axisDirections = Vec3d(1,1,-1);

	public:
		VRCartesianKinematics();
		~VRCartesianKinematics();

		static VRCartesianKinematicsPtr create();
		VRCartesianKinematicsPtr ptr();

		void setComponents(VRTransformPtr aX, VRTransformPtr aY, VRTransformPtr aZ);
		void setAxisParams(int dirX, int dirY, int dirZ);
		void setEndEffector(PosePtr pose) override;
		void setOrigin(PosePtr pos);
};

OSG_END_NAMESPACE;

#endif //VRCARTESIANKINEMATICS_H_INCLUDED
