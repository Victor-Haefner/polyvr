#include "VRWiringSimulation.h"
#include "VRElectricComponent.h"
#include "VRElectricSystem.h"
#include "VRWire.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;

VRWiringSimulation::VRWiringSimulation(VRElectricSystemPtr s) : system(s) {}
VRWiringSimulation::~VRWiringSimulation() {}

VRWiringSimulationPtr VRWiringSimulation::create(VRElectricSystemPtr s) { return VRWiringSimulationPtr( new VRWiringSimulation(s) ); }
VRWiringSimulationPtr VRWiringSimulation::ptr() { return static_pointer_cast<VRWiringSimulation>(shared_from_this()); }

void VRWiringSimulation::iterate() {
	for (auto& c : system->getComponents()) { // reset current;
        auto component = c.second;
		for (auto& p : component->ports) component->setCurrent("0", p.first);
		for (auto& c : component->connections) c->entity->set("current", "0");
	}

	auto propagateConnection = [&](VRElectricComponentPtr component, VRWirePtr connection, list<VRElectricComponentPtr>& stack, int debug = 1) {
		if (!connection) return;
		if (debug) connection->entity->set("current", "1");

		auto target = connection->getOther(component);
		auto components = system->getRegistred(target.ecadID);
		for (auto c : components) {
            stack.push_back(c);
            c->setCurrent("1",target.port);
		}
	};

	auto propagate = [&](VRElectricComponentPtr component, list<VRElectricComponentPtr>& stack, int debug) {
		for (auto& connection : component->connections ) {
			propagateConnection(component, connection, stack, debug);
		}
	};

	VRElectricComponentPtr L = system->getRegistred("=KVE120+-XPWR")[0];
	L->setCurrent("1", "0");
	list<VRElectricComponentPtr> stack = {L};
	auto R = rand();

	while (stack.size() > 0 ) {
		VRElectricComponentPtr component = stack.back();
		stack.pop_back();

		// set processed flag, sure about that?;
		if (component->flag == R) continue;
		component->flag = R;

		// get entity;
		auto e = component->entity;
		if (!e) continue;

		auto en = e->getName();

		if (e->is_a("ConnectorComponent") ) propagate(component, stack, 1);

		if (e->is_a("DoublePushbutton") ) {
			if (e->get("state", 0)->getValue() == "unpressed") { // reversed switch;
				propagateConnection(component, component->getConnection("1"), stack);
				propagateConnection(component, component->getConnection("2"), stack);
			}
			if (e->get("state", 1)->getValue() == "pressed" ) {
				propagateConnection(component, component->getConnection("3"), stack);
				propagateConnection(component, component->getConnection("4"), stack);
			}
			continue;
		}

		if (e->is_a("Switch") ) {
			if (e->get("state")->getValue() == "pressed") propagate(component, stack, 1);
		}
	};
}
