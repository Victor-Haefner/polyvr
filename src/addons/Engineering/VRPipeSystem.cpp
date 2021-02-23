#include "VRPipeSystem.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/math/graph.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRConcept.h"

using namespace OSG;

template<> string typeName(const VRPipeSystem& m) { return "PipeSystem"; }

float gasSpeed = 10;


// Pipe Segment ----

VRPipeSegment::VRPipeSegment(float radius, float length) : radius(radius), length(length) {
    area = Pi*radius*radius;
    volume = area*length;
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(float radius, float length) { return VRPipeSegmentPtr( new VRPipeSegment(radius, length) ); }

void VRPipeSegment::mixPressure(float& pressure, float otherVolume, float dt) {
    float dP = this->pressure - pressure;
    lastPressureDelta = 0;
    if (abs(dP) < 1e-3) {
        pressure += dP*0.5;
        this->pressure -= dP*0.5;
        return;
    }

    float dV = dP*area*dt*gasSpeed; // volume delta through the pipe section area
    if (volume-dV < 0) dV = volume;
    float newP = this->pressure*(volume-dV)/volume;
    lastPressureDelta = newP-this->pressure;
    this->pressure += lastPressureDelta;
    pressure *= (otherVolume+dV)/otherVolume;
    //cout << "mixPressure pipe: " << this->pressure << ", tank: " << pressure << " V: " << otherVolume << endl;
}

void VRPipeSegment::addPressure(float performance, float dt) {
    float dV = performance*dt; // volume delta from pump
    float newP = this->pressure*(volume+dV)/volume;
    lastPressureDelta = newP-this->pressure;
    this->pressure += lastPressureDelta;
}


// Pipe Node ----

VRPipeNode::VRPipeNode(VREntityPtr entity) : entity(entity) {}
VRPipeNode::~VRPipeNode() {}

VRPipeNodePtr VRPipeNode::create(VREntityPtr entity) { return VRPipeNodePtr( new VRPipeNode(entity) ); }


// Pipe System ----

VRPipeSystem::VRPipeSystem() : VRGeometry("pipeSystem") {
    graph = Graph::create();
    initOntology();

    updateCb = VRUpdateCb::create("pipesSimUpdate", bind(&VRPipeSystem::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRPipeSystem::~VRPipeSystem() {}

VRPipeSystemPtr VRPipeSystem::create() { return VRPipeSystemPtr( new VRPipeSystem() ); }
VRPipeSystemPtr VRPipeSystem::ptr() { return static_pointer_cast<VRPipeSystem>(shared_from_this()); }

VROntologyPtr VRPipeSystem::getOntology() { return ontology; }

int VRPipeSystem::addNode(PosePtr pos, string type, map<string, string> params) {
    auto e = ontology->addEntity("pipeNode", type);
    auto n = VRPipeNode::create(e);
    for (auto& p : params) e->set(p.first, p.second);
    int nID = graph->addNode(pos);
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
            pipe1->lastPressureDelta = 0;
            pipe2->lastPressureDelta = 0;
            if (abs(dP) < 1e-3) {
                pipe1->pressure += dP*0.5;
                pipe2->pressure -= dP*0.5;
                continue;
            }

            float area = valveRadius*valveRadius*Pi;
            float dV = dP*area*dt*gasSpeed; // volume delta through the pipe section area
            float newP1 = pipe1->pressure*(pipe1->volume+dV)/pipe1->volume;
            float newP2 = pipe2->pressure*(pipe2->volume-dV)/pipe2->volume;
            pipe1->lastPressureDelta = newP1-pipe1->pressure;
            pipe2->lastPressureDelta = newP2-pipe2->pressure;
            pipe1->pressure += pipe1->lastPressureDelta;
            pipe2->pressure += pipe2->lastPressureDelta;
            //cout << " valve dV: " << pipe1->lastPressureDelta << " " << pipe2->lastPressureDelta << endl;
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


    for (auto n : nodes) { // print some stats
        auto entity = n.second->entity;
        float P = entity->getValue("pressure", 1.0);
        float V = entity->getValue("volume", 1.0);
        if (entity->is_a("Tank")) cout << " tank: P " << P << " V " << V << endl;
    }

    for (auto s : segments) { // print some stats
        cout << " pipe: P " << s.second->pressure << " V " << s.second->volume << endl;
    }

    updateVisual();
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

void VRPipeSystem::setDoVisual(bool b) { doVisual = b; }

void VRPipeSystem::setValve(int nID, bool b) { nodes[nID]->entity->set("state", toString(b)); }

void VRPipeSystem::updateVisual() {
    if (!doVisual) return;

    VRGeoData data(ptr());
    auto s = data.size();

    if (s == 0) {
        Vec3d norm(0,1,0);
        Color3f white(1,1,1);

        for (auto& s : segments) {
            auto edge = graph->getEdge(s.first);

            auto p1 = graph->getPosition(edge.from);
            auto p2 = graph->getPosition(edge.to);

            data.pushVert(p1->pos(), norm, white);
            data.pushVert(p2->pos(), norm, white);
            data.pushLine();
        }

        for (auto& n : nodes) {
            auto p = graph->getPosition(n.first);
            data.pushVert(p->pos(), norm, white);
            data.pushPoint();
        }

        auto m = VRMaterial::create("pipes");
        m->setLineWidth(5);
        m->setLit(0);
        m->addPass();
        m->setPointSize(10);
        m->setLit(0);
        setMaterial(m);

        cout << "apply data: " << data.size() << endl;
        data.apply(ptr());
    }

    // update system state

    int i=0;

    for (auto& s : segments) {
        Color3f c(0,0,1);

        auto p = s.second->lastPressureDelta;
        if (abs(p) > 1e-5) {
            if (p > 0) c = Color3f(1, 0, 0);
            if (p < 0) c = Color3f(0, 1, 0);
        }

        data.setColor(i, c); i++;
        data.setColor(i, c); i++;
    }

    for (auto& n : nodes) {
        Color3f c(1,1,0);

        if (n.second->entity->is_a("Valve")) {
            s = n.second->entity->getValue("state", false);
            c = s ? Color3f(0,1,0) : Color3f(1,0,0);
        }

        data.setColor(i, c); i++;
    }
}

