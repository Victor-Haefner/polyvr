#include "VRElectricComponent.h"
#include "VRElectricSystem.h"
#include "VRWire.h"
#include "addons/Engineering/Programming/VRLADVariable.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;

VRElectricComponent::VRElectricComponent(string name, string eID, string mID) {
    static size_t i = 0; i++;
    vrID = i;

    setName(name);
    setEcadID(eID);
    setMcadID(mID);
}

VRElectricComponent::~VRElectricComponent() {}

VRElectricComponentPtr VRElectricComponent::create(string name, string eID, string mID) { return VRElectricComponentPtr( new VRElectricComponent(name, eID, mID) ); }
VRElectricComponentPtr VRElectricComponent::ptr() { return static_pointer_cast<VRElectricComponent>(shared_from_this()); }

void VRElectricComponent::setCurrent(string current, string port) {
    if (!ports.count(port)) return;
    auto& p = ports[port];
    p.entity->set("current", current);
    if (p.ladHWaddr == "") return;

    auto sys = system.lock();
    if (!sys) return;
    sys->setVariable(p.ladHWaddr, toInt(current));
}

VRWirePtr VRElectricComponent::getWire(VRElectricComponentPtr c2) {
    for (auto& c : connections) {
        if (c->source.ecadID == c2->ecadID) return c;
        if (c->target.ecadID == c2->ecadID) return c;
    }
    return 0;
}

VRWirePtr VRElectricComponent::getConnection(string port) {
    if (!ports.count(port)) return 0;
    return ports[port].connection;
}

void VRElectricComponent::registerID(string ID) {
    if (auto sys = system.lock()) sys->registerID(ID, ptr());
}

void VRElectricComponent::setName(string n) {
    if (n != "") {
        name = n;
        registerID(n);
    }
}

void VRElectricComponent::setEcadID(string eID) {
    if (eID != "") {
        ecadID = eID;
        registerID(eID);
    }
}

void VRElectricComponent::setMcadID(string mID) {
    if (mID != "") {
        mcadID = mID;
        registerID(mID);
    }
}
