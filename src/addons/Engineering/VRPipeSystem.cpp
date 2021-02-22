#include "VRPipeSystem.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRConcept.h"

using namespace OSG;

template<> string typeName(const VRPipeSystem& m) { return "PipeSystem"; }


// Pipe Segment ----

VRPipeSegment::VRPipeSegment(float radius, float length) : radius(radius), length(length) {
    area = Pi*radius*radius;
    volume = area*length;
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(float radius, float length) { return VRPipeSegmentPtr( new VRPipeSegment(radius, length) ); }

void VRPipeSegment::mixPressure(float& pressure, float otherVolume, float dt) {
    float dP = this->pressure - pressure;
    if (abs(dP) < 1e-3) return;
    float dV = dP*area*dt; // volume delta through the pipe section area
    this->pressure *= (volume+dV)/volume;
    pressure *= (otherVolume+dV)/otherVolume;
}

void VRPipeSegment::addPressure(float performance, float dt) {
    float dV = performance*dt; // volume delta from pump
    this->pressure *= (volume+dV)/volume;
}


// Pipe Node ----

VRPipeNode::VRPipeNode(VREntityPtr entity) : entity(entity) {}
VRPipeNode::~VRPipeNode() {}

VRPipeNodePtr VRPipeNode::create(VREntityPtr entity) { return VRPipeNodePtr( new VRPipeNode(entity) ); }


// Pipe System ----

VRPipeSystem::VRPipeSystem() {
    initOntology();
}

VRPipeSystem::~VRPipeSystem() {}

VRPipeSystemPtr VRPipeSystem::create() { return VRPipeSystemPtr( new VRPipeSystem() ); }
VRPipeSystemPtr VRPipeSystem::ptr() { return static_pointer_cast<VRPipeSystem>(shared_from_this()); }

VROntologyPtr VRPipeSystem::getOntology() { return ontology; }

int VRPipeSystem::addNode(string type) {
    auto e = ontology->addEntity("pipeNode", type);
    auto n = VRPipeNode::create(e);
    int nID = graph->addNode();
    nodes[nID] = n;
    return nID;
}

int VRPipeSystem::addSegment(float radius, float length, int n1, int n2) {
    int sID = graph->connect(n1, n2);
    auto s = VRPipeSegment::create(radius, length);
    segments[sID] = s;
    return sID;
}

vector<VRPipeSegmentPtr> VRPipeSystem::getPipes(int nID) {
    vector<VRPipeSegmentPtr> res;
    for (auto e : graph->getInEdges (nID) ) res.push_back(segments[e.ID]);
    for (auto e : graph->getOutEdges(nID) ) res.push_back(segments[e.ID]);
    return res;
}

vector<VRPipeSegmentPtr> VRPipeSystem::getInPipes(int nID) {
    vector<VRPipeSegmentPtr> res;
    for (auto e : graph->getInEdges (nID) ) res.push_back(segments[e.ID]);
    return res;
}

vector<VRPipeSegmentPtr> VRPipeSystem::getOutPipes(int nID) {
    vector<VRPipeSegmentPtr> res;
    for (auto e : graph->getOutEdges(nID) ) res.push_back(segments[e.ID]);
    return res;
}

void VRPipeSystem::update() {
    float dt = 1.0/60; // TODO: use animation

    for (auto n : nodes) { // traverse nodes, change pressure in segments
        int nID = n.first;
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            float tankVolume = entity->getValue("volume", 0.0);
            float tankPressure = entity->getValue("pressure", 1.0);
            for (auto p : getPipes(nID)) p->mixPressure(tankPressure, tankVolume, dt);
            entity->set("pressure", toString(tankPressure));
            continue;
        }

        if (entity->is_a("Pump")) {
            float pumpPerformance = entity->getValue("performance", 0.0);
            for (auto p : getInPipes(nID))  p->addPressure(-pumpPerformance, dt);
            for (auto p : getOutPipes(nID)) p->addPressure( pumpPerformance, dt);
            continue;
        }

        if (entity->is_a("Valve")) {
            bool valveState = entity->getValue("state", false);
            float valveRadius = entity->getValue("radius", 0.0);
            if (valveState == 0) continue; // valve closed
            auto pipes = getPipes(nID);
            if (pipes.size() != 2) continue;

            auto pipe1 = pipes[0];
            auto pipe2 = pipes[1];
            float dP = pipe2->pressure - pipe1->pressure;
            if (dP < 1e-3) continue;

            float area = valveRadius*valveRadius*Pi;
            pipe1->pressure -= dP*area*dt;
            pipe2->pressure += dP*area*dt;
            continue;
        }

        if (entity->is_a("Outlet")) {
            float outletRadius = entity->getValue("radius", 0.0);
            auto pipes = getPipes(nID);
            if (pipes.size() != 1) continue;
            auto pipe = pipes[0];

            float area = outletRadius*outletRadius*Pi;
            pipe->pressure -= (pipe->pressure-1.0)*area*dt;
            continue;
        }
    }
}

void VRPipeSystem::initOntology() {
    ontology = VROntology::create("PipeSystem");
    auto Tank = ontology->addConcept("Tank");
    auto Pump = ontology->addConcept("Pump");
    auto Outlet = ontology->addConcept("Outlet");
    auto Valve = ontology->addConcept("Valve", "Outlet");

    Tank->addProperty("pressure", "float");
    Tank->addProperty("volume", "float");
    Pump->addProperty("performance", "float");
    Outlet->addProperty("radius", "float");
    Valve->addProperty("state", "bool");
}


