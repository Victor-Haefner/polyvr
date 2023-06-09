#include "VRElectricVisualization.h"
#include "VRElectricComponent.h"
#include "VRElectricSystem.h"
#include "VRWire.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "core/math/partitioning/graph.h"
#include "addons/Algorithms/VRGraphLayout.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

VRElectricVisualization::VRElectricVisualization() : VRObject("eVisual") {}
VRElectricVisualization::~VRElectricVisualization() {}

VRElectricVisualizationPtr VRElectricVisualization::create() {
    auto v = VRElectricVisualizationPtr( new VRElectricVisualization() );
    v->init();
    return v;
}

VRElectricVisualizationPtr VRElectricVisualization::ptr() { return static_pointer_cast<VRElectricVisualization>(shared_from_this()); }

void VRElectricVisualization::setSystem(VRElectricSystemPtr s) { system = s; }

void VRElectricVisualization::init() {
    cableViz = VRAnalyticGeometry::create("wiring");
	cableViz->setLabelParams(0.01, 1, 1, Color4f(0.2,0.2,0.2,1), Color4f(0.8,0.8,0.8,1), Vec3d(-0.03,0,0.01));
	addChild(cableViz);
}

void VRElectricVisualization::addModuleViz(string module, Color4f c) {
    modulesViz[module] = VRAnalyticGeometry::create("machineViz");
    modulesViz[module]->setLabelParams(0.02, 1, 1, Color4f(0,0,0,1), c, Vec3d(-0.03,0.1,0.01));
    addChild(modulesViz[module]);
}

void VRElectricVisualization::clear() {
	cableViz->clear();
	for (auto m : modulesViz) m.second->clear();

    vizEGraph.reset();
    vizPGraph.reset();
    vizEGraphData.clear();
    vizPGraphData.clear();
    vizEGraphMap.clear();
    vizPGraphMap.clear();
    vizEGraphFixed.clear();
    vizPGraphFixed.clear();
}

void VRElectricVisualization::computeGraphs() {
	vizEGraph = Graph::create();
	vizPGraph = Graph::create();

	map<VRElectricComponentPtr, bool> visibleComponents;
	map<string, Vec3d> cPositions;

	auto drawComponent = [&](VRElectricComponentPtr component, Vec3d p, bool fixed) {
		visibleComponents[component] = 1;
		cPositions[component->getEcadID()] = p;

		auto nID = vizEGraph->addNode(Pose::create(p));
		vizEGraphData[nID] = component;
		vizEGraphMap[component->getEcadID()] = nID;
		if (fixed) vizEGraphFixed.push_back(nID);

		nID = vizPGraph->addNode(Pose::create(p));
		vizPGraphData[nID] = component;
		vizPGraphMap[component->getEcadID()] = nID;
		if (fixed) vizPGraphFixed.push_back(nID);
	};

	//temporary improvising for start XPWR/"Steckdose"-------------;
	auto eRoot = system->getRegistred("=KVE120+-XPWR")[0];
	drawComponent(eRoot, Vec3d(-20, 2, -1), 1);

	//1-> initial positioning, make MCAD-ECAD-Geometry visible;
	for (auto& c : system->getComponents()) {
        auto component = c.second;
		if (component->getMcadID() != "" && component->getGeometry()) {
			auto g = dynamic_pointer_cast<VRTransform>( component->getGeometry() );
			if (g) {
                Vec3d p = g->getWorldPosition();
                drawComponent(component, p, 1);
            }
		}
	}

	auto getCPos = [&](VRElectricComponentPtr component) {
		if (cPositions.count(component->getEcadID())) return cPositions[component->getEcadID()];
		return component->getPosition();
	};

	auto isConnViz = [](int i, int j, map<int, vector<int>>& d) {
		if (d.count(i)) {
            auto& v = d[i];
            if (::find(v.begin(), v.end(), j) != v.end()) return true;
		}
		if (d.count(j)) {
            auto& v = d[j];
            if (::find(v.begin(), v.end(), i) != v.end()) return true;
		}
		return false;
	};

	auto setConViz = [](int i, int j, map<int, vector<int>>& d) {
		if (!d.count(i)) d[i] = vector<int>();
		d[i].push_back(j);
	};

	auto connectComponents = [](VRElectricComponentPtr c1, VRElectricComponentPtr c2,
                             map<VRElectricComponentPtr,vector<VRElectricComponentPtr>>& wires,
                             map<string, int>& Map, GraphPtr graph) {
		if (wires.count(c1)) {
            auto& v = wires[c1];
            if (::find(v.begin(), v.end(), c2) != v.end()) return;
		} else wires[c1] = vector<VRElectricComponentPtr>();
		wires[c1].push_back(c2);
		int nID1 = Map[c1->getEcadID()];
		int nID2 = Map[c2->getEcadID()];
		graph->connect(nID1, nID2);
	};

	map<VRElectricComponentPtr,bool> allVisibleECADcomps;
	map<VRElectricComponentPtr,vector<VRElectricComponentPtr>> allVisibleEWires;
	map<VRElectricComponentPtr,vector<VRElectricComponentPtr>> allVisiblePWires;
	map<int, vector<int>> visualEConnections; // replace by wire IDs!!;
	map<int, vector<int>> visualPConnections; // replace by wire IDs!!;

	auto vizPath = [&](vector<VRElectricComponentPtr> path, bool isBus, int i, int j) {
		int pathLength = path.size();
		if (pathLength <= 1) return;
		if (isBus) setConViz(i,j, visualPConnections);
		else setConViz(i,j, visualEConnections);

		Vec3d P0 = getCPos(path[0]);
		Vec3d P1 = getCPos(path[pathLength-1]);

		//set unique position for each ecad component in path;
		for (int i=0; i<pathLength; i++) {
            auto& part = path[i];
			if (!allVisibleECADcomps.count(part)) {
				allVisibleECADcomps[part] = true;
				Vec3d p = P0 + (P1-P0)*(float(i)/(pathLength-1));
				auto component = system->getRegistred(part->getEcadID())[0];
				Vec3d P2 = getCPos(component);
				if (P2.length() < 1e-5 ) drawComponent(component, p, 0);
			}
		}

		for (int s = 0; s<pathLength-1; s++) { //add subvector for each ecad component within path;
			if (!isBus) connectComponents(path[s], path[s+1], allVisibleEWires, vizEGraphMap, vizEGraph);
			else        connectComponents(path[s], path[s+1], allVisiblePWires, vizPGraphMap, vizPGraph);
		}
	};

	vector<VRElectricComponentPtr> comps;
	for (auto c : system->getComponents()) comps.push_back(c.second);

	for (size_t i=0; i<comps.size(); i++) {
        for (size_t j=0; j<comps.size(); j++) {
            auto c1 = comps[i];
            auto c2 = comps[j];
			if (c1 == c2) continue;
			bool hasRepr1 = bool(c1->getGeometry()) || bool(visibleComponents.count(c1));
			bool hasRepr2 = bool(c2->getGeometry()) || bool(visibleComponents.count(c2));
			if (hasRepr1 && hasRepr2) {
				if (!isConnViz(i,j,visualPConnections) ) {
					auto bPath = system->getBusRoute(c1->getEcadID(), c2->getEcadID());
					vizPath(bPath, true, i, j);
				}
				if (!isConnViz(i,j,visualEConnections) ) {
					auto ePath = system->getElectricRoute(c1->getEcadID(), c2->getEcadID());
					vizPath(ePath, false, i, j);
				}
			}
		}
	}


	// relax graph
	auto layout = VRGraphLayout::create();
	layout->setGraph(vizEGraph);
	layout->setRadius(0.1);
	layout->setAlgorithm("SPRINGS");
	//layout->setAlgorithm("OCCUPANCYMAP");
	for (auto nID : vizEGraphFixed) layout->fixNode(nID);
	layout->compute(1000,0.01);
}

void VRElectricVisualization::drawNodes() {
	auto getComponentViz = [&](string machine) {
		if (!modulesViz.count(machine)) {
			Color4f c(1,0.9,0.9,1);
			if (machine == "=KVE120+") c = Color4f(0.9,1,0.9,1);
            addModuleViz(machine, c);
		}
		return modulesViz[machine];
	};

	auto getClassColor = [](VRElectricComponentPtr component) {
		if (!component->getEntity()) return Color3f(0.2,0.2,0.2);
		auto e = component->getEntity();
		if (e->is_a("PLC")) return Color3f(1,1,0);
		if (e->is_a("FrequencyConverter")) return Color3f(0.4,0.3,0.1);
		if (e->is_a("Fuse")) return Color3f(0,0,1);
		if (e->is_a("Switch")) return Color3f(0.6,0.6,0.6);
		if (e->is_a("AutomationModule")) return Color3f(1,0.5,0);
		return Color3f(0.5,0,0); //others;
	};

	for (auto node : vizEGraph->getNodesCopy() ) {
        auto nID = node.ID;
		auto component = vizEGraphData[nID];
		auto p = vizEGraph->getPosition(nID);
		auto c = getClassColor(component);
		//if nID in vizEGraphFixed: c = [1,1,1];

		auto lbl = component->getAddress().component;
		if (lbl == "") lbl = component->getAddress().socket;
		if (lbl == "") lbl = component->getEcadID();

		auto viz = getComponentViz(component->getAddressMachine());
		auto& v = vizEGraphFixed;
		if (::find(v.begin(), v.end(), nID) != v.end())
            viz->addVector(p->pos(), Vec3d(), c, lbl, 0);
		else viz->addVector(p->pos(), Vec3d(), c, lbl, 1);
	}
}

void VRElectricVisualization::drawWires() {
	// electric wires
	if (vizEGraph) {
        for (auto e : vizEGraph->getEdgesCopy()) {
            auto c1 = vizEGraphData[e.from];
            auto c2 = vizEGraphData[e.to];
            auto p1 = vizEGraph->getPosition(e.from);
            auto p2 = vizEGraph->getPosition(e.to);

            auto wire = c1->getWire(c2);
            float current = 0;
            auto we = wire->getEntity();
            if (we) current = toFloat(we->get("current")->getValue());
            Color3f c(1-current,current,0);
            cableViz->addVector(p1->pos(), p2->pos()-p1->pos(), c, wire->getLabel(), 0);
        }
	}

	// bus wires
	if (vizPGraph) {
        for (auto e : vizPGraph->getEdgesCopy()) {
            auto c1 = vizPGraphData[e.from];
            auto c2 = vizPGraphData[e.to];
            int f = vizEGraphMap[c1->ecadID];
            int t = vizEGraphMap[c2->ecadID];
            auto p1 = vizEGraph->getPosition(f);
            auto p2 = vizEGraph->getPosition(t);
            Color3f c(0.2,0.3,1);
            auto wire = c1->getWire(c2);
            cableViz->addVector(p1->pos() + Vec3d(0,0,0.02), p2->pos()-p1->pos(), c, wire->getLabel(), 0);
        }
	}
}

void VRElectricVisualization::updateWires() {
    if (!isVisible()) return;
    cableViz->clear();
    drawWires();
}

void VRElectricVisualization::update() {
    if (!isVisible()) return;

    clear();
    computeGraphs();
    drawNodes();
    drawWires();
}
