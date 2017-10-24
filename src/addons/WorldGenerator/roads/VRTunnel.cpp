#include "VRTunnel.h"


VRTunnel::VRTunnel() {}
VRTunnel::~VRTunnel() {}

VRTunnelPtr VRTunnel::create() { return VRTunnelPtr(new VRTunnel()); }
