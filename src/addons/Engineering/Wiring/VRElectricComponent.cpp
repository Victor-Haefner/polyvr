#include "VRElectricComponent.h"
#include "VRElectricSystem.h"
#include "VRWire.h"
#include "addons/Engineering/Programming/VRLADVariable.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;

VRElectricComponent::VRElectricComponent(VRElectricSystemPtr sys, string name, string eID, string mID) {
    static size_t i = 0; i++;
    vrID = i;
    system = sys;
}

VRElectricComponent::~VRElectricComponent() {}

VRElectricComponentPtr VRElectricComponent::create(VRElectricSystemPtr sys, string name, string eID, string mID) {
    auto c = VRElectricComponentPtr( new VRElectricComponent(sys, name, eID, mID) );
    c->setName(name);
    c->setEcadID(eID);
    c->setMcadID(mID);
    return c;
}

VRElectricComponentPtr VRElectricComponent::ptr() { return static_pointer_cast<VRElectricComponent>(shared_from_this()); }

string VRElectricComponent::getName() { return name; }
string VRElectricComponent::getEcadID() { return ecadID; }
string VRElectricComponent::getMcadID() { return mcadID; }
int VRElectricComponent::getEGraphID() { return egraphID; }
int VRElectricComponent::getPGraphID() { return pgraphID; }
VRObjectPtr VRElectricComponent::getGeometry() { return geometry; }
VREntityPtr VRElectricComponent::getEntity() { return entity; }
vector<VRWirePtr> VRElectricComponent::getConnections() { return connections; }
VRWirePtr VRElectricComponent::getPortWire(string port) { return ports[port].connection; }
VREntityPtr VRElectricComponent::getPortEntity(string port) { return ports[port].entity; }
void VRElectricComponent::setPosition(Vec3d p) { position = p; }
Vec3d VRElectricComponent::getPosition() { return position; }
VRElectricComponent::Address VRElectricComponent::getAddress() { return address; }

vector<string> VRElectricComponent::getPorts() {
    vector<string> res;
    for (auto s : ports) res.push_back(s.first);
    return res;
}

void VRElectricComponent::setGeometry(VRObjectPtr obj) { geometry = obj; }
void VRElectricComponent::setEntity(VREntityPtr e) { entity = e; }
void VRElectricComponent::setEGraphID(int ID) { egraphID = ID; }
void VRElectricComponent::setPGraphID(int ID) { pgraphID = ID; }
void VRElectricComponent::setPortEntity(string port, VREntityPtr e) { ports[port].entity = e; }

void VRElectricComponent::addPort(string port, string ladHWaddr, string ecadHWaddr, string socket) {
    ports[port] = Port();
    ports[port].name = port;
    ports[port].ladHWaddr = ladHWaddr;
    ports[port].ecadHWaddr = ecadHWaddr;
    ports[port].socket = socket;
}

bool VRElectricComponent::hasAddress() { return bool(address.address != ""); }
string VRElectricComponent::getAddressMachine() { return address.machine; }

void VRElectricComponent::setCurrent(string current, string port) {
    if (!ports.count(port)) return;
    auto& p = ports[port];
    p.entity->set("current", current);
    if (p.ladHWaddr == "") return;

    auto sys = system.lock();
    if (!sys) return;
    sys->setVariable(p.ladHWaddr, current);
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
