#include "VRBridge.h"

using namespace OSG;

VRBridge::VRBridge() {}
VRBridge::~VRBridge() {}

VRBridgePtr VRBridge::create() { return VRBridgePtr(new VRBridge()); }
