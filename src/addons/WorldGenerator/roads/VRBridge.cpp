#include "VRBridge.h"


VRBridge::VRBridge() {}
VRBridge::~VRBridge() {}

VRTunnelPtr VRBridge::create() { return VRTunnelPtr(new VRBridge()); }
