#include "VRLADVariable.h"

using namespace OSG;

VRLADVariable::VRLADVariable() {}
VRLADVariable::~VRLADVariable() {}

VRLADVariablePtr VRLADVariable::create() { return VRLADVariablePtr( new VRLADVariable() ); }
VRLADVariablePtr VRLADVariable::ptr() { return static_pointer_cast<VRLADVariable>(shared_from_this()); }
