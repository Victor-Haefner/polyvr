#include "VRCartesianKinematics.h"

VRCartesianKinematics::VRCartesianKinematics() {}
VRCartesianKinematics::~VRCartesianKinematics() {}

VRCartesianKinematicsPtr create() { return VRCartesianKinematicsPtr( new VRCartesianKinematics() ); }
VRCartesianKinematicsPtr ptr() { return shared_from_this(); }
