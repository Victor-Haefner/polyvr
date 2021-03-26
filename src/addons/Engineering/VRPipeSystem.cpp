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

double gasSpeed = 300;


// Pipe Segment ----

VRPipeSegment::VRPipeSegment(double radius, double length) : radius(radius), length(length) {
    area = Pi*radius*radius;
    volume = area*length;
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(double radius, double length) { return VRPipeSegmentPtr( new VRPipeSegment(radius, length) ); }

void VRPipeSegment::addEnergy(double m, double d) {
    lastPressureDelta = m/volume;
    pressure += lastPressureDelta;

    if (m>0) density = (density * volume + m*d) / (volume + m); // only when adding material with different density
}

void VRPipeSegment::handleTank(double& otherPressure, double otherVolume, double& otherDensity, double dt) {
    double dP = pressure - otherPressure;
    double m = dP*area*dt*gasSpeed; // energy through the pipe section area

    if (dP > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of tank
        m = min(m, otherPressure*otherVolume); // not more than available!
    }

    addEnergy(-m, otherDensity);
    otherPressure += m/otherVolume;
    if (m>0) otherDensity = (otherDensity * otherVolume + m*density) / (otherVolume + m); // only when adding material with different density
    //cout << "handleTank " << dP << " " << pressure << " " << otherPressure << endl;
}

void VRPipeSegment::handleValve(double area, VRPipeSegmentPtr other, double dt) {
    double dP = pressure - other->pressure;
    area = min(area, min(this->area, other->area));
    double m = dP*area*dt*gasSpeed; // energy through the valve opening

    if (dP > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of other
        m = min(m, other->pressure*other->volume); // not more than available!
    }

    addEnergy(-m, other->density);
    other->addEnergy(m, density);
}

void VRPipeSegment::handlePump(double performance, VRPipeSegmentPtr other, double dt) {
    double v = pressure/(other->pressure + pressure);
    double m = performance*dt/exp(1/v);
    m = min(m, pressure*volume); // pump out not more than available!
    addEnergy(-m, other->density);
    other->addEnergy(m, density);
    //cout << " pump " << dP << " m " << m << " v " << v << endl;
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

int VRPipeSystem::addNode(string name, PosePtr pos, string type, map<string, string> params) {
    auto e = ontology->addEntity(name, type);
    auto n = VRPipeNode::create(e);
    for (auto& p : params) e->set(p.first, p.second);
    int nID = graph->addNode(pos);
    nodes[nID] = n;
    nodesByName[name] = nID;
    return nID;
}

int VRPipeSystem::getNode(string name) { return nodesByName[name]; }
int VRPipeSystem::getSegment(int n1, int n2) { return graph->getEdgeID(n1, n2); }

int VRPipeSystem::addSegment(double radius, int n1, int n2) {
    int sID = graph->connect(n1, n2);
    auto p1 = graph->getPosition(n1)->pos();
    auto p2 = graph->getPosition(n2)->pos();
    double length = (p2-p1).length();
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

void VRPipeSystem::printSystem() {
    double totalEnergy = 0;
    for (auto n : nodes) { // print some stats
        auto entity = n.second->entity;
        double P = entity->getValue("pressure", 1.0);
        double V = entity->getValue("volume", 1.0);
        if (entity->is_a("Tank")) cout << " tank: P " << P << " V " << V << endl;
        totalEnergy += P*V;
    }

    for (auto s : segments) { // print some stats
        double P = s.second->pressure;
        double V = s.second->volume;
        cout << " pipe: P " << P << " V " << V << endl;
        totalEnergy += P*V;
    }
    cout << " total energy: " << totalEnergy << endl;
}

void VRPipeSystem::update() {
    int subSteps = 10;
    double dT = 1.0/60; // TODO: use animation
    double dt = dT/subSteps; // TODO: use animation

    for (int i=0; i<subSteps; i++) {

        for (auto n : nodes) { // traverse nodes, change pressure in segments
            int nID = n.first;
            auto node = n.second;
            auto entity = node->entity;

            if (entity->is_a("Tank")) {
                double tankVolume = entity->getValue("volume", 0.0);
                double tankPressure = entity->getValue("pressure", 1.0);
                double tankDensity = entity->getValue("density", 1.0);
                for (auto p : getPipes(nID)) p->handleTank(tankPressure, tankVolume, tankDensity, dt);
                entity->set("pressure", toString(tankPressure));
                entity->set("density", toString(tankDensity));
                continue;
            }

            if (entity->is_a("Junction")) { // just averages pressures, TODO: compute energy exchange with timestep
                auto pipes = getPipes(nID);
                double commonEnergy = 0;
                double commonVolume = 0;
                double commonDensity = 0;

                for (auto p : pipes) {
                    commonEnergy += p->pressure*p->volume;
                    commonVolume += p->volume;
                    commonDensity += p->density*p->volume;
                }

                double avrgPressure = commonEnergy/commonVolume;
                double avrgDensity = commonDensity/commonVolume;

                for (auto p : pipes) {
                    p->pressure = avrgPressure;
                    p->density = avrgDensity;
                }

                continue;
            }

            if (entity->is_a("Pump")) {
                auto pipes = getPipes(nID);
                if (pipes.size() != 2) continue;
                auto pipe1 = pipes[0];
                auto pipe2 = pipes[1];

                double pumpPerformance = entity->getValue("performance", 0.0);
                pipe1->handlePump(pumpPerformance, pipe2, dt);
                continue;
            }

            if (entity->is_a("Valve")) {
                bool valveState = entity->getValue("state", false);
                double valveRadius = entity->getValue("radius", 0.0);
                if (valveState == 0) continue; // valve closed
                auto pipes = getPipes(nID);
                if (pipes.size() != 2) continue;

                auto pipe1 = pipes[0];
                auto pipe2 = pipes[1];
                double area = valveRadius*valveRadius*Pi;
                pipe1->handleValve(area, pipe2, dt);
                continue;
            }

            if (entity->is_a("Outlet")) {
                double outletRadius = entity->getValue("radius", 0.0);
                auto pipes = getPipes(nID);
                if (pipes.size() != 1) continue;
                auto pipe = pipes[0];

                double area = outletRadius*outletRadius*Pi;
                pipe->pressure -= (pipe->pressure-1.0)*area*dt;
                continue;
            }
        }
    }

    //printSystem();
    updateVisual();
}

void VRPipeSystem::initOntology() {
    ontology = VROntology::create("PipeSystem");
    auto Tank = ontology->addConcept("Tank");
    auto Pump = ontology->addConcept("Pump");
    auto Outlet = ontology->addConcept("Outlet");
    auto Junction = ontology->addConcept("Junction");
    auto Valve = ontology->addConcept("Valve", "Outlet");

    Tank->addProperty("pressure", "double");
    Tank->addProperty("volume", "double");
    Tank->addProperty("density", "double");
    Pump->addProperty("performance", "double");
    Outlet->addProperty("radius", "double");
    Valve->addProperty("state", "bool");
}

void VRPipeSystem::setDoVisual(bool b) { doVisual = b; }

double VRPipeSystem::getSegmentPressure(int i) { return segments[i]->pressure; }
double VRPipeSystem::getTankPressure(string n) { return nodes[nodesByName[n]]->entity->getValue("pressure", 1.0); }
double VRPipeSystem::getTankDensity(string n) { return nodes[nodesByName[n]]->entity->getValue("density", 1.0); }
double VRPipeSystem::getTankVolume(string n) { return nodes[nodesByName[n]]->entity->getValue("volume", 1.0); }
double VRPipeSystem::getPump(string n) { return nodes[nodesByName[n]]->entity->getValue("performance", 0.0); }

void VRPipeSystem::setValve(string n, bool b)  { nodes[nodesByName[n]]->entity->set("state", toString(b)); }
void VRPipeSystem::setPump(string n, double p) { nodes[nodesByName[n]]->entity->set("performance", toString(p)); }
void VRPipeSystem::setTankPressure(string n, double p) { nodes[nodesByName[n]]->entity->set("pressure", toString(p)); }
void VRPipeSystem::setTankDensity(string n, double p) { nodes[nodesByName[n]]->entity->set("density", toString(p)); }

void VRPipeSystem::updateVisual() {
    if (!doVisual) return;

    VRGeoData data(ptr());
    auto s = data.size();

    Vec3d dO = Vec3d(-0.1,-0.1,-0.1);

    if (s == 0) {
        Vec3d norm(0,1,0);
        Color3f white(1,1,1);
        Color3f yellow(1,1,0);

        for (auto& s : segments) {
            auto edge = graph->getEdge(s.first);

            auto p1 = graph->getPosition(edge.from);
            auto p2 = graph->getPosition(edge.to);

            data.pushVert(p1->pos(), norm, white);
            data.pushVert(p2->pos(), norm, white);
            data.pushLine();
            data.pushVert(p1->pos()+dO, norm, yellow);
            data.pushVert(p2->pos()+dO, norm, yellow);
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
        //float pdelta = s.second->lastPressureDelta; // last written delta, not the correct one
        float pressure = s.second->pressure;
        float density = s.second->density;

        // show pdelta
        /*Color3f c1(0,0,1);
        Color3f c2(0,0,1);
        if (pdelta > 0) c2 = Color3f(1, 0, 0);
        if (pdelta < 0) c2 = Color3f(0, 1, 0);
        double t = abs(pdelta/pressure * 100.0);
        if (t > 1) t = 1;
        Color3f c = c2*t + c1*(1-t);*/

        // show pressure
        float S = 1.0/2; // color scale above 1 bar
        float t = 1.0 - pressure; // around 1 bar
        Color3f c;
        if (t > 0) c = Color3f(0, 0, 1-t); // below 1 bar
        else c = Color3f(-t*S, 0, 1+t*S);

        data.setColor(i, c); i++;
        data.setColor(i, c); i++;

        // density
        float D = 1.0; // color scale above 1
        Color3f cd;
        float d = 1.0 - density; // around 1
        if (d > 0) cd = Color3f(1-d, 1-d, 0); // below 1
        else cd = Color3f(1, 1, -d*D);

        data.setColor(i, cd); i++;
        data.setColor(i, cd); i++;
    }

    for (auto& n : nodes) {
        Color3f c(0.4,0.4,0.4);

        if (n.second->entity->is_a("Valve")) {
            bool s = n.second->entity->getValue("state", false);
            c = s ? Color3f(0,1,0) : Color3f(1,0,0);
        }

        if (n.second->entity->is_a("Junction")) {
            c = Color3f(0.2,0.4,1);
        }

        if (n.second->entity->is_a("Pump")) {
            double p = n.second->entity->getValue("performance", 0.0);
            c = p>1e-3 ? Color3f(1,1,0) : Color3f(1,0.5,0);
        }

        data.setColor(i, c); i++;
    }
}

