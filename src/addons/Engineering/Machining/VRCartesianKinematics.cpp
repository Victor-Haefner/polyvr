#include "VRCartesianKinematics.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRCartesianKinematics& t) { return "CartesianKinematics"; }

VRCartesianKinematics::VRCartesianKinematics() {}
VRCartesianKinematics::~VRCartesianKinematics() {}

VRCartesianKinematicsPtr VRCartesianKinematics::create() { return VRCartesianKinematicsPtr( new VRCartesianKinematics() ); }
VRCartesianKinematicsPtr VRCartesianKinematics::ptr() { return static_pointer_cast<VRCartesianKinematics>(shared_from_this()); }
