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

float gasSpeed = 300;


// Pipe Segment ----

VRPipeSegment::VRPipeSegment(float radius, float length) : radius(radius), length(length) {
    area = Pi*radius*radius;
    volume = area*length;
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(float radius, float length) { return VRPipeSegmentPtr( new VRPipeSegment(radius, length) ); }

void VRPipeSegment::addMass(float m) {
    lastPressureDelta = m/volume;
    pressure += lastPressureDelta;
}

void VRPipeSegment::handleTank(float& otherPressure, float otherVolume, float dt) {
    float dP = pressure - otherPressure;
    float m = dP*area*dt*gasSpeed; // mass through the pipe section area

    if (dP > 0) { // mass is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // mass going out of tank
        m = min(m, otherPressure*otherVolume); // not more than available!
    }

    addMass(-m);
    otherPressure = otherPressure + m/otherVolume;
    //cout << "handleTank " << dP << " " << pressure << " " << otherPressure << endl;
}

void VRPipeSegment::handleValve(float area, VRPipeSegmentPtr other, float dt) {
    float dP = pressure - other->pressure;
    area = min(area, min(this->area, other->area));
    float m = dP*area*dt*gasSpeed; // mass through the valve opening

    if (dP > 0) { // mass is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // mass going out of other
        m = min(m, other->pressure*other->volume); // not more than available!
    }

    addMass(-m);
    other->addMass(m);
}

void VRPipeSegment::handlePump(float performance, VRPipeSegmentPtr other, float dt) {
    float m = performance*dt;
    m = min(m, pressure*volume); // pump out not more than available!
    addMass(-m);
    other->addMass(m);
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
    int subSteps = 10;
    float dT = 1.0/60; // TODO: use animation
    float dt = dT/subSteps; // TODO: use animation

    for (int i=0; i<subSteps; i++) {

        for (auto n : nodes) { // traverse nodes, change pressure in segments
            int nID = n.first;
            auto node = n.second;
            auto entity = node->entity;

            if (entity->is_a("Tank")) {
                float tankVolume = entity->getValue("volume", 0.0);
                float tankPressure = entity->getValue("pressure", 1.0);
                for (auto p : getPipes(nID)) p->handleTank(tankPressure, tankVolume, dt);
                entity->set("pressure", toString(tankPressure));
                continue;
            }

            if (entity->is_a("Junction")) { // just averages pressures, TODO: compute mass exchange with timestep
                auto pipes = getPipes(nID);
                float commonMass = 0;
                float commonVolume = 0;
                for (auto p : pipes) {
                    commonMass += p->pressure*p->volume;
                    commonVolume += p->volume;
                }
                float avrgPressure = commonMass/commonVolume;
                for (auto p : pipes) p->pressure = avrgPressure;
                continue;
            }

            if (entity->is_a("Pump")) {
                auto pipes = getPipes(nID);
                if (pipes.size() != 2) continue;
                auto pipe1 = pipes[0];
                auto pipe2 = pipes[1];

                float pumpPerformance = entity->getValue("performance", 0.0);
                pipe1->handlePump(pumpPerformance, pipe2, dt);
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
                float area = valveRadius*valveRadius*Pi;
                pipe1->handleValve(area, pipe2, dt);
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

    float totalMass = 0;
    for (auto n : nodes) { // print some stats
        auto entity = n.second->entity;
        float P = entity->getValue("pressure", 1.0);
        float V = entity->getValue("volume", 1.0);
        if (entity->is_a("Tank")) cout << " tank: P " << P << " V " << V << endl;
        totalMass += P*V;
    }

    for (auto s : segments) { // print some stats
        float P = s.second->pressure;
        float V = s.second->volume;
        cout << " pipe: P " << P << " V " << V << endl;
        totalMass += P*V;
    }
    cout << " total mass: " << totalMass << endl;

    updateVisual();
}

void VRPipeSystem::initOntology() {
    ontology = VROntology::create("PipeSystem");
    auto Tank = ontology->addConcept("Tank");
    auto Pump = ontology->addConcept("Pump");
    auto Outlet = ontology->addConcept("Outlet");
    auto Junction = ontology->addConcept("Junction");
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
        Color3f c1(0,0,1);
        Color3f c2(0,0,1);

        auto p = s.second->lastPressureDelta;
        if (p > 0) c2 = Color3f(1, 0, 0);
        if (p < 0) c2 = Color3f(0, 1, 0);

        float t = abs(p/s.second->pressure * 100.0);
        if (t > 1) t = 1;
        Color3f c = c2*t + c1*(1-t);

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

