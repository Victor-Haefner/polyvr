#include "VRMachiningCode.h"

VRMachiningCode::VRMachiningCode() {}
VRMachiningCode::~VRMachiningCode() {}

VRMachiningCodePtr create() { return VRMachiningCodePtr( new VRMachiningCode() ); }
VRMachiningCodePtr ptr() { return shared_from_this(); }
