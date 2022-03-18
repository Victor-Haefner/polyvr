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

VRElectricComponent::Address VRWire::getSource() { return source; }
VRElectricComponent::Address VRWire::getTarget() { return target; }

void VRWire::setEntity(VREntityPtr e) { entity = e; }
VREntityPtr VRWire::getEntity() { return entity; }
string VRWire::getLabel() { return label; }
string VRWire::getType() { return cType; }
