#include "VRMachiningCode.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRMachiningCode& t) { return "MachiningCode"; }

VRMachiningCode::VRMachiningCode() {}
VRMachiningCode::~VRMachiningCode() {}

VRMachiningCodePtr VRMachiningCode::create() { return VRMachiningCodePtr( new VRMachiningCode() ); }
VRMachiningCodePtr VRMachiningCode::ptr() { return static_pointer_cast<VRMachiningCode>(shared_from_this()); }

void VRMachiningCode::readGCode(string path) {
	;
}