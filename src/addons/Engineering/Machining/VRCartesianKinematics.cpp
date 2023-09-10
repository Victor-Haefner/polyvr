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
	int dX = axisDirections[0];
	int dY = axisDirections[1];
	int dZ = axisDirections[2];
	if (axisX) { auto p = axisX->getFrom(); axisX->setFrom(Vec3d(dX*x, p[1], p[2])); }
	if (axisY) { auto p = axisY->getFrom(); axisY->setFrom(Vec3d(p[0], dY*y, p[2])); }
	if (axisZ) { auto p = axisZ->getFrom(); axisZ->setFrom(Vec3d(p[0], p[1], dZ*z)); }
	if (axisY == axisZ && axisZ) { auto p = axisZ->getFrom(); axisZ->setFrom(Vec3d(p[0], dY*y, dZ*z)); }
}

void VRCartesianKinematics::setAxisParams(int dirX, int dirY, int dirZ){
    cout<<"setting axis parameters"<<endl;
    axisDirections[0] = dirX;
    axisDirections[1] = dirY;
    axisDirections[2] = dirZ;
}



