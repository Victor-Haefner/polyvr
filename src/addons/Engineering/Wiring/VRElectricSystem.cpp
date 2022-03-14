#include "VRElectricSystem.h"
#include "VRElectricComponent.h"
#include "VRWire.h"
#include "VRWiringSimulation.h"
#include "core/math/partitioning/graph.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/objects/object/VRObject.h"
#include "core/scene/VRScene.h"
#include "addons/Engineering/Programming/VRLADVariable.h"

using namespace OSG;

VRElectricSystem::VRElectricSystem() {}
VRElectricSystem::~VRElectricSystem() {}

VRElectricSystemPtr VRElectricSystem::create() {
    auto s = VRElectricSystemPtr( new VRElectricSystem() );
    s->simulation = VRWiringSimulation::create(s);
    return s;
}

VRElectricSystemPtr VRElectricSystem::ptr() { return static_pointer_cast<VRElectricSystem>(shared_from_this()); }

VRElectricComponentPtr VRElectricSystem::getComponent(size_t ID) { return components[ID]; }
map<size_t, VRElectricComponentPtr> VRElectricSystem::getComponents() { return components; }
map<string, vector<VRElectricComponentPtr>> VRElectricSystem::getComponentIDs() { return componentIDs; }
vector<VRElectricComponentPtr> VRElectricSystem::getRegistred(string ID) { return componentIDs[ID]; }
VRLADVariablePtr VRElectricSystem::getVariable(string ID) { return profinetVariables[ID]; }
map<string, VRObjectPtr> VRElectricSystem::getObjectsByName() { return objectsByName; }

void VRElectricSystem::simECAD() {
    simulation->iterate();
}

VRElectricComponentPtr VRElectricSystem::newComponent(string name, string eID, string mID) {
    auto c = VRElectricComponent::create(ptr(), name, eID, mID);
    components[c->vrID] = c;
    return c;
}

void VRElectricSystem::setVariable(string HWaddr, string ci) {
    for (auto& var : profinetVariables) {
        auto& v = var.second;
        if (v->name == "SFT_Estop_Button" || v->name == "OS_ExtMot_Overload_Switch") continue;
        if (v->logicalAddress == HWaddr && v->value != ci ) v->value = ci;
    }
}

void VRElectricSystem::setLADVariables(map<string, VRLADVariablePtr> vars) {
    profinetVariables = vars;
}

void VRElectricSystem::registerID(string ID, VRElectricComponentPtr c) {
    if (!componentIDs.count(ID)) componentIDs[ID] = vector<VRElectricComponentPtr>();
    componentIDs[ID].push_back(c);
}

void VRElectricSystem::buildECADgraph() {
    electricGraph = Graph::create();
    profinetGraph = Graph::create();
    componentsByEGraphID.clear();
    componentsByPGraphID.clear();

	// add all components as graph nodes;
	for (auto c : components) {
        auto component = c.second;
		auto p = Pose::create();
		auto neID = electricGraph->addNode(p);
		auto npID = profinetGraph->addNode(p);
		component->egraphID = neID;
		component->pgraphID = npID;
		componentsByEGraphID[neID] = component;
		componentsByPGraphID[npID] = component;
	}

	// add component connections to graph;
	for (auto c : components) {
        auto component = c.second;
		for (auto connection : component->connections) {
			string cType = connection->cType;
			auto component1 = componentIDs[connection->source.ecadID][0];
			auto component2 = componentIDs[connection->target.ecadID][0];

			if (cType == "electric") { // birectional connection;
				electricGraph->connect(component1->egraphID, component2->egraphID);
				electricGraph->connect(component2->egraphID, component1->egraphID);
			}

			if (cType == "profinet") { // birectional connection;
				profinetGraph->connect(component1->pgraphID, component2->pgraphID);
				profinetGraph->connect(component2->pgraphID, component1->pgraphID);
			}
		}
	}
}

void VRElectricSystem::importECAD() {
	auto join = [&](string plantID1, string plantID2, string id1, string kind, string id2) -> string {
		if (plantID1 == "" || (id1 == "" && id2 == "")) return "";

		string seq1 = plantID1 + "+" + plantID2;
		string seq2 = id1 + kind + id2;

		string c = "-";
		if (plantID2 == "") {
			c = "+-";
            seq1 = plantID1;
		}

		return "=" + seq1 + c + seq2;
	};

	/*auto split = [&](string s, string c) -> vector<string> {
		if (s == "") return {"", ""};
		auto data = splitString(s,c);
		int N = data.size();
		for (int i=2; i<N; i++) data[1] += ":" + data[i];
		if (N >= 2) return { data[0], data[1] };
		if (N == 1) return { data[0], "" };
		return {"", ""};
	};*/

	auto parseAddress = [&](string address) {
		VRElectricComponent::Address a(address);
		auto s1 = splitString(address, "-");
		int Ns1 = s1.size();
		a.machine = s1[0];
		a.ecadID = splitString(address, ":")[0];

		if (Ns1 == 2 || (Ns1 == 3 && s1[2] == "")) {
			auto s2 = splitString(s1[1], ":");
			a.socket = s2[0];
			if (s2.size() > 1) a.port = s2[1];
		}

		if (Ns1 == 3 && s1[2] != "") {
			a.component = s1[1];
			if (a.component.back() == ')') a.component.pop_back();
			auto s2 = splitString(s1[2], ":");
			a.socket = s2[0];
			if (s2.size() > 1) a.port = s2[1];
			a.ecadID = a.machine+"-"+a.component;
		}

		return a;
	};

	auto isProfinet = [&](const VRElectricComponent::Address& address) { // TODO;
		// case XPi, i a number;
		for (int i = 0; i<10; i++) {
		    string I = toString(i);
            if (address.port == "XP"+I || address.port == "XPN"+I) return true;
            if (address.socket == "XP"+I || address.socket == "XPN"+I) return true;
		}

		// case X1:P;
		if (address.port != "") {
			if (contains(address.socket, "X1") && address.port[0] == 'P') return true;
		}

		// case XFP3;
		if (address.socket == "XFP3") return true;
		return false;
	};

	// get ecad components;
	auto bmk = XML::create();
	bmk->read("data/ECAD/EDC/bmk.edc");
	for (auto node : bmk->getRoot()->getChildren("O17", true) ) {
		string ecadID = node->getAttribute("P20006");
		string name = node->getAttribute("P20100_1");
		if (name == "") continue;
		if (componentIDs.count(ecadID)) continue;
		auto component = newComponent(name, ecadID, "");
		component->address = parseAddress(ecadID);
	}

	// get ecad wires;
	auto edges = XML::create();
	edges->read("data/ECAD/EDC/Verbindungen.edc");
	for (auto edge : edges->getRoot()->getChildren("O18", true) ) {
		string targetData = edge->getAttribute("P31020");
		string sourceData = edge->getAttribute("P31019");
		string label  = edge->getAttribute("P31011");

		auto source = parseAddress(sourceData);
		auto target = parseAddress(targetData);

		if (label == "") label = "default";
		if (source.port == "PE" || target.port == "PE") continue ; // ignore grounding;

		auto connection = VRWire::create();
		connection->target = target;
		connection->source = source;
		connection->label  = label;
		connection->cType  = "electric";
		if (isProfinet(source) || isProfinet(target)) connection->cType = "profinet";

		if (!componentIDs.count( source.ecadID )) {
            auto component = newComponent("", source.ecadID, "");
			component->address = parseAddress(source.ecadID);
		}

		if (!componentIDs.count( target.ecadID )) {
            auto component = newComponent("", target.ecadID, "");
			component->address = parseAddress(target.ecadID);
		}

		for (auto component : componentIDs[source.ecadID] ) {
			component->connections.push_back(connection);
		}

		for (auto component : componentIDs[target.ecadID] ) {
			component->connections.push_back(connection);
		}
	}

	auto ecadProject = XML::create();
	ecadProject->read("data/ECAD/EPJ/217155 SOBCO Algerien Ball Forming Line.epj");
	//ecadProject = VR->importXML("data/ECAD/EPJ/Extruder->epj");
	for (auto i : ecadProject->getRoot()->getChildren("O117", true) ) {
		for (auto j : i->getChildren("P11", true)) { // get ecad -> mcad mapping data;
			if (!j->hasAttribute("P22003") || !j->hasAttribute("P22001")) continue;
			auto mcadID = j->getAttribute("P22003");
			auto name = j->getAttribute("P22001");
			if (componentIDs.count(name)) {
				for (auto component : componentIDs[name]) { //get mcadID from ECAD Project;
					component->setMcadID(mcadID);
				}
			}
		}
	}

	function<void(VRObjectPtr)> traverseScene = [&](VRObjectPtr obj) {
		auto name = obj->getName();
		objectsByName[name] = obj;
		for (auto child : obj->getChildren()) traverseScene(child);
	};
	traverseScene(VRScene::getCurrent()->getRoot());

	//make 100% sure each component has correct ecadID and mcadID based on traverse scene
	for (auto i : ecadProject->getRoot()->getChildren("O117", true) ) {
		for (auto j : i->getChildren("P11", true) ) {
			if (!j->hasAttribute("P22001") || !j->hasAttribute("P22003")) continue;
			auto mcadID = j->getAttribute("P22003");
			auto name = j->getAttribute("P22001");
            //if (mcadID == "0656 012 03") cout << " -- O117 > P11 " << name << "  ,  " << mcadID << endl;
			for (auto obj : objectsByName) {
                //if (mcadID == "0656 012 03") cout << "    " << contains(obj.first, mcadID) << "   " << obj.first << endl;
				if (contains(obj.first, mcadID)) { //if mcadID is not in traversed scene;
                    //cout << " -- " << obj.first << endl;
					for (auto s : ecadProject->getRoot()->getChildren("O17", true)) { //get correct mcadID;
						for (auto t : s->getChildren("P11", true) ) {
							for (auto attrib : t->getAttributes() ) {
								if (name == attrib.second) {
                                    //cout << " --B" << endl;
									for (auto l : s->getChildren("P150", true) ) {
										auto plant1 = l->getAttribute("P1100");
										auto plant2 = l->getAttribute("P1200");
										auto id1 = l->getAttribute("P20012");
										auto kind = l->getAttribute("P20013");
										auto id2 = l->getAttribute("P20014");

										auto corrspd_ecadID = join(plant1,plant2,id1,kind,id2);

										for (auto compts : componentIDs) {
											if (corrspd_ecadID == compts.second[0]->ecadID ) {
												compts.second[0]->mcadID = mcadID;
												compts.second[0]->name = name;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// get ecad -> LAD mapping data;
	map<string, map<string, string>> O117_data;
	for (auto i : ecadProject->getRoot()->getChildren("O117", true) ) {
		string name;
		vector<pair<string, string>> IN_O117;

		for (auto j : i->getChildren("P11", true) ) {
			if (j->hasAttribute("P21002_1") && j->hasAttribute("P21000_1")) {
				auto IN = j->getAttribute("P21002_1");
				auto PO = j->getAttribute("P21000_1");
				IN_O117.push_back( make_pair(IN,PO) );
				cout << "IN_O117 " << IN << ", " << PO << endl;
			}
			if (j->hasAttribute("P22001")) name = j->getAttribute("P22001");
		}

		if (IN_O117.size() > 0 && name != "") {
			if (!O117_data.count(name)) O117_data[name] = map<string, string>();
			for (auto& a : IN_O117 ) O117_data[name][a.first] = a.second;
		}
	}

	map<string, map<string, string>> O17_data;
	for (auto s : ecadProject->getRoot()->getChildren("O17", true) ) {
		string ecadID;
		vector<pair<string, string>> IN_O17;

		for (auto l : s->getChildren("P150", true)) { // get ecad ID;
			string plant1 = l->getAttribute("P1100"); //;
			string plant2 = l->getAttribute("P1200"); //;
			string id1 = l->getAttribute("P20012"); //;
			string kind = l->getAttribute("P20013"); //;
			string id2 = l->getAttribute("P20014"); //;
			ecadID = join(plant1,plant2,id1,kind,id2);
		}

		for (auto t : s->getChildren("P11", true) ) {
			if (t->hasAttribute("P20407") && t->hasAttribute("P20400") ) {
				string IN = t->getAttribute("P20407");
				string AD = t->getAttribute("P20400");
				IN_O17.push_back( make_pair(IN,AD) );
			}
		}

		if (IN_O17.size() > 0 && ecadID != "") {
			if (!O17_data.count(ecadID)) O17_data[ecadID] = map<string, string>();
			for (auto& a : IN_O17) {
				O17_data[ecadID][a.first] = a.second;
			}
		}
	}

	// add module ports
	for (auto data : O117_data ) {
        auto name = data.first;
        auto dataO117 = data.second;
		if (componentIDs.count(name)) {
			auto component = componentIDs[name][0];
			if (O17_data.count(component->ecadID)) {
				auto dataO17 = O17_data[component->ecadID];
				for (auto IN : dataO117 ) {
					if (dataO17.count(IN.first)) {
						auto LADaddr = dataO17[IN.first];
						if (LADaddr[0] == 'E') LADaddr = "%I"+subString(LADaddr,1);
						if (LADaddr[0] == 'A') LADaddr = "%O"+subString(LADaddr,1);
						VRElectricComponent::Port p(dataO117[IN.first]);
						p.ladHWaddr = LADaddr;
						p.ecadHWaddr = dataO17[IN.first];
						p.socket = IN.first;
						component->ports[p.name] = p;
					}
				}
			}
		}
	}

	auto getPort = [&](const map<string, VRElectricComponent::Port>& others) -> string {
		for (int i=0; i<50; i++) {
			string k = toString(i);
			if (others.count(k)) continue;
			return k;
		}
		return "";
	};

	auto handleAddress = [&](VRElectricComponentPtr component, VRWirePtr connection, VRElectricComponent::Address& address) {
        if (address.ecadID == component->ecadID ) {
            if (address.port == "") address.port = getPort(component->ports);
            if ( component->ports.count(address.port) ) {
                if (!component->ports[address.port].connection ) {
                    component->ports[address.port].connection = connection;
                }
                if (component->ports[address.port].connection == connection) return;

                int i = 0;
                string P = address.port;
                while( component->ports.count(address.port) ) {
                    address.port = P + toString(i);
                    i++;
                }
            }

            VRElectricComponent::Port p(address.port);
            p.connection = connection;
            component->ports[p.name] = p;
        }
	};

	// add component ports
	for (auto comp : components ) {
        auto component = comp.second;
		for (auto connection : component->connections ) {
            handleAddress(component, connection, connection->source);
            handleAddress(component, connection, connection->target);
		}
	}
}
