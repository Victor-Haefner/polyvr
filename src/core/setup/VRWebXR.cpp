#include "VRWebXR.h"

using namespace OSG;

VRWebXR::VRWebXR() {}
VRWebXR::~VRWebXR() {}

VRWebXRPtr VRWebXR::create() { return VRWebXRPtr( new VRWebXR() ); }
VRWebXRPtr VRWebXR::ptr() { return static_pointer_cast<VRWebXR>(shared_from_this()); }
