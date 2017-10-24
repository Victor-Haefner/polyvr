#include "VRTunnel.h"

using namespace OSG;

VRTunnel::VRTunnel() {}
VRTunnel::~VRTunnel() {}

VRTunnelPtr VRTunnel::create() { return VRTunnelPtr(new VRTunnel()); }
