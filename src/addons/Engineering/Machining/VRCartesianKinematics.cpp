#include "VRCartesianKinematics.h"

#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/objects/VRTransform.h"

#include <OpenSG/OSGVector.h>

using namespace OSG;

VRCartesianKinematics::VRCartesianKinematics() {}
VRCartesianKinematics::~VRCartesianKinematics() {}

VRCartesianKinematicsPtr VRCartesianKinematics::create() { return VRCartesianKinematicsPtr( new VRCartesianKinematics() ); }
VRCartesianKinematicsPtr VRCartesianKinematics::ptr() { return static_pointer_cast<VRCartesianKinematics>(VRMachiningKinematics::ptr()); }

void VRCartesianKinematics::setComponents(VRTransformPtr aX, VRTransformPtr aY, VRTransformPtr aZ) {
	axisX = aX;
	axisY = aY;
	axisZ = aZ;
}

void VRCartesianKinematics::setEndEffector(PosePtr pose) {
	if (!axisX) cout << "Error in VRCartesianKinematics::setEndEffector: no axisX" << endl;
	if (!axisY) cout << "Error in VRCartesianKinematics::setEndEffector: no axisY" << endl;
	if (!axisZ) cout << "Error in VRCartesianKinematics::setEndEffector: no axisZ" << endl;
	double x = pose->pos()[0];
	double y = pose->pos()[1];
	double z = pose->pos()[2];
	if (axisX) axisX->setFrom(Vec3d(x, 0, 0));
	if (axisY) axisY->setFrom(Vec3d(0, y, 0));
	if (axisZ) axisZ->setFrom(Vec3d(0, 0, -z));
	if (axisY == axisZ && axisZ) axisZ->setFrom(Vec3d(0, y, -z));
}

