#include "VRWire.h"
#include "VRElectricComponent.h"

using namespace OSG;

VRWire::VRWire() {}
VRWire::~VRWire() {}

VRWirePtr VRWire::create() { return VRWirePtr( new VRWire() ); }
VRWirePtr VRWire::ptr() { return static_pointer_cast<VRWire>(shared_from_this()); }

VRElectricComponent::Address VRWire::getThis(VRElectricComponentPtr first) {
    if (target.ecadID == first->ecadID) return target;
    return source;
}

VRElectricComponent::Address VRWire::getOther(VRElectricComponentPtr first) {
    if (target.ecadID == first->ecadID) return source;
    return target;
}
