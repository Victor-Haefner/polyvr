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
    computeGeometry();
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(double radius, double length) { return VRPipeSegmentPtr( new VRPipeSegment(radius, length) ); }

void VRPipeSegment::setLength(double l) {
    length = l;
    computeGeometry();
}

void VRPipeSegment::computeGeometry() {
    area = Pi*radius*radius;
    volume = area*length;
}

void VRPipeSegment::addEnergy(double m, double d, bool p1) {
    if (p1) pressure1 += m/volume;
    else pressure2 += m/volume;

    if (m>0) { // only when adding material with different density
        density = (density * volume + m*d) / (volume + m);
    }
}

void VRPipeSegment::handleTank(double& otherPressure, double otherVolume, double& otherDensity, double dt, bool p1) {
    double pressure = p1 ? pressure1 : pressure2;
    double dP = pressure - otherPressure;
    double m = dP*area*dt*gasSpeed; // energy through the pipe section area

    if (dP > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of tank
        m = min(m, otherPressure*otherVolume); // not more than available!
    }

    //cout << "handleTank p1: " << p1 << " dP: " << dP << " P: " << pressure1 << "->" << pressure2 ;
    addEnergy(-m, otherDensity, p1);
    otherPressure += m/otherVolume;
    if (m>0) otherDensity = (otherDensity * otherVolume + m*density) / (otherVolume + m); // only when adding material with different density
    //cout << " ... " <<  " P: " << pressure << "->" << pressure1 << "->" << pressure2 << " pipe: " << this << endl;
}

double VRPipeSegment::computeExchange(double hole, VRPipeSegmentPtr other, double dt, bool p1) {
    double pressure = p1 ? pressure1 : pressure2;
    double otherPressure = p1 ? other->pressure2 : other->pressure1;
    double dP = pressure - otherPressure;
    hole = min(hole, min(this->area, other->area));
    double m = dP*hole*dt*gasSpeed; // energy through the opening

    if (dP > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of other
        m = min(m, otherPressure*other->volume); // not more than available!
    }

    return m;
}

void VRPipeSegment::handleValve(double area, VRPipeSegmentPtr other, double dt, bool p1) {
    double m = computeExchange(area, other, dt, p1);
    addEnergy(-m, other->density, p1);
    other->addEnergy(m, density, !p1);
}

void VRPipeSegment::handlePump(double performance, bool isOpen, VRPipeSegmentPtr other, double dt, bool p1) {
    double pressure = p1 ? pressure1 : pressure2;
    double otherPressure = p1 ? other->pressure2 : other->pressure1;
    double v = pressure/(otherPressure + pressure);
    double m = performance*dt/exp(1/v);
    //if (isOpen) m = max(m, computeExchange(area*0.1, other, dt, p1)); // minimal exchange if pump is open
    m = min(m, pressure*volume); // pump out not more than available!
    addEnergy(-m, other->density, p1);
    other->addEnergy(m, density, !p1);
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
    rebuildMesh = true;
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
    rebuildMesh = true;
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
        double P1 = s.second->pressure1;
        double P2 = s.second->pressure2;
        double V = s.second->volume;
        cout << " pipe: P " << P1 << "->" << P2 << " \tFl: " << s.second->flow << endl;
        totalEnergy += (P1+P2)*0.5*V;
    }
    cout << " total energy: " << totalEnergy << endl;
}

void VRPipeSystem::update() {
    //printSystem();
    //sleep(1);

    int subSteps = 10;
    double dT = 1.0/60; // TODO: use animation
    double dt = dT/subSteps;

    for (int i=0; i<subSteps; i++) {

        for (auto n : nodes) { // traverse nodes, change pressure in segments
            int nID = n.first;
            auto node = n.second;
            auto entity = node->entity;

            if (entity->is_a("Tank")) {
                double tankVolume = entity->getValue("volume", 0.0);
                double tankPressure = entity->getValue("pressure", 1.0);
                double tankDensity = entity->getValue("density", 1.0);
                for (auto p : getInPipes(nID)) p->handleTank(tankPressure, tankVolume, tankDensity, dt, false);
                for (auto p : getOutPipes(nID)) p->handleTank(tankPressure, tankVolume, tankDensity, dt, true);
                entity->set("pressure", toString(tankPressure));
                entity->set("density", toString(tankDensity));
                continue;
            }

            if (entity->is_a("Junction")) { // just averages pressures, TODO: compute energy exchange with timestep
                auto inPipes = getInPipes(nID);
                auto outPipes = getOutPipes(nID);
                double commonEnergy = 0;
                double commonVolume = 0;
                double commonDensity = 0;

                for (auto p : inPipes) {
                    commonEnergy += p->pressure2*p->volume*0.5;
                    commonVolume += p->volume*0.5;
                    commonDensity += p->density*p->volume*0.5;
                }
                for (auto p : outPipes) {
                    commonEnergy += p->pressure1*p->volume*0.5;
                    commonVolume += p->volume*0.5;
                    commonDensity += p->density*p->volume*0.5;
                }

                double avrgPressure = commonEnergy/commonVolume;
                double avrgDensity = commonDensity/commonVolume;

                for (auto p : inPipes) {
                    //p->pressure1 = avrgPressure; // test
                    p->pressure2 = avrgPressure;
                    p->density = avrgDensity;
                }
                for (auto p : outPipes) {
                    //p->pressure2 = avrgPressure; // test
                    p->pressure1 = avrgPressure;
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
                bool pumpIsOpen= entity->getValue("isOpen", false);
                pipe1->handlePump(pumpPerformance, pumpIsOpen, pipe2, dt, false);
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
                pipe1->handleValve(area, pipe2, dt, false);
                continue;
            }

            if (entity->is_a("Outlet")) {
                double outletRadius = entity->getValue("radius", 0.0);
                auto pipes = getPipes(nID);
                if (pipes.size() != 1) continue;
                auto pipe = pipes[0];

                bool p1 = getOutPipes(nID).size() == 1;

                double area = outletRadius*outletRadius*Pi;
                if (p1) pipe->pressure1 -= (pipe->pressure1-1.0)*area*dt;
                else    pipe->pressure2 -= (pipe->pressure2-1.0)*area*dt;
                continue;
            }
        }

        for (auto s : segments) { // compute pressure relaxation
            double dP = s.second->pressure2 - s.second->pressure1; // compute pressure gradient
            double kmax = dP*0.5;
            double k = kmax/(1.0+s.second->length)*min(1.0, dt*gasSpeed);
            s.second->pressure1 += k;
            s.second->pressure2 -= k;
        }

        for (auto s : segments) { // compute flows accellerations
            double m = s.second->volume*s.second->density;
            double dP = s.second->pressure2 - s.second->pressure1; // compute pressure gradient
            double F = dP*s.second->area;
            double R = s.second->density * s.second->flow ; // friction
            double a = (F-R)/m; // accelleration
            s.second->dFl = a*dt*s.second->area;  // pipe flow change in m³ / s
        }

        int itr = 0;
        bool flowCheck = true;
        while (flowCheck) { // check flow changes until nothing changed
            itr++;
            if (itr > 10) break;
            flowCheck = false;
            for (auto n : nodes) { // traverse nodes, change pressure in segments
                int nID = n.first;
                auto node = n.second;
                auto entity = node->entity;

                if (entity->is_a("Outlet")) continue; // does nothing to a flow

                if (entity->is_a("Pump") || entity->is_a("Valve")) { // check for closed pumps and valves
                    auto pipes = getPipes(nID);
                    if (pipes.size() != 2) continue;
                    auto pipe1 = pipes[0];
                    auto pipe2 = pipes[1];

                    bool closed = false;
                    if (entity->is_a("Pump")) {
                        double pumpPerformance = entity->getValue("performance", 0.0);
                        bool pumpIsOpen = entity->getValue("isOpen", false);
                        closed = (pumpPerformance < 1e-3 && !pumpIsOpen);
                    }
                    if (entity->is_a("Valve")) {
                        bool valveState = entity->getValue("state", false);
                        closed = (valveState == 0);
                    }

                    if (closed) {
                        pipe1->dFl = -pipe1->flow;
                        pipe2->dFl = -pipe2->flow;
                        flowCheck = true;
                        continue;
                    }
                }


                vector<int> inFlowPipeIDs;
                vector<int> outFlowPipeIDs;
                for (auto e : graph->getInEdges(nID) ) {
                    auto pipe = segments[e.ID];
                    if (pipe->dFl > 0) inFlowPipeIDs.push_back(e.ID);
                    if (pipe->dFl < 0) outFlowPipeIDs.push_back(e.ID);
                }
                for (auto e : graph->getOutEdges(nID) ) {
                    auto pipe = segments[e.ID];
                    if (pipe->dFl > 0) outFlowPipeIDs.push_back(e.ID);
                    if (pipe->dFl < 0) inFlowPipeIDs.push_back(e.ID);
                }

                double maxInFlow = 0;
                double maxOutFlow = 0;
                for (auto eID : inFlowPipeIDs  ) maxInFlow  += abs(segments[eID]->flow + segments[eID]->dFl);
                for (auto eID : outFlowPipeIDs ) maxOutFlow += abs(segments[eID]->flow + segments[eID]->dFl);
                double maxFlow = min(maxInFlow, maxOutFlow);
                if (maxFlow > 1e-6) {
                    double inPart  = maxFlow/maxInFlow;
                    double outPart = maxFlow/maxOutFlow;
                    for (auto eID :  inFlowPipeIDs ) segments[eID]->dFl *= inPart;
                    for (auto eID : outFlowPipeIDs ) segments[eID]->dFl *= outPart;
                    //for (auto eID :  inFlowPipeIDs ) segments[eID]->dFl = (segments[eID]->flow + segments[eID]->dFl)*inPart  - segments[eID]->flow;
                    //for (auto eID : outFlowPipeIDs ) segments[eID]->dFl = (segments[eID]->flow + segments[eID]->dFl)*outPart - segments[eID]->flow;
                    if (abs(inPart-1.0) > 1e-3 || abs(outPart-1.0) > 1e-3) flowCheck = true;
                } else { // no flow
                    for (auto eID :  inFlowPipeIDs ) segments[eID]->dFl = -segments[eID]->flow;
                    for (auto eID : outFlowPipeIDs ) segments[eID]->dFl = -segments[eID]->flow;
                    flowCheck = true;
                }
                continue;
            }
        }

        cout << "flows" << endl;
        for (auto s : segments) { // add final flow accellerations
            s.second->flow += s.second->dFl;  // pipe flow change in m³ / s
            cout << " flow +" << s.second->dFl << " -> " << s.second->flow << endl;
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
    Pump->addProperty("isOpen", "bool");
    Outlet->addProperty("radius", "double");
    Valve->addProperty("state", "bool");
}

void VRPipeSystem::setNodePose(int nID, PosePtr p) {
    graph->setPosition(nID, p);

    // update pipe lengths
    for (auto e : graph->getInEdges (nID) ) {
        double l = (graph->getPosition(e.from)->pos() - p->pos()).length();
        auto pipe = segments[e.ID];
        pipe->setLength(l);
    }
    for (auto e : graph->getOutEdges (nID) ) {
        double l = (graph->getPosition(e.to)->pos() - p->pos()).length();
        auto pipe = segments[e.ID];
        pipe->setLength(l);
    }

    rebuildMesh = true;
}

int VRPipeSystem::insertSegment(int nID, int sID, float radius) {
    int cID = disconnect(nID, sID);
    addSegment(radius, nID, cID);
    return cID;
}

int VRPipeSystem::disconnect(int nID, int sID) {
    rebuildMesh = true;
    int cID = graph->split(nID, sID);
    auto e = ontology->addEntity("junction", "Junction");
    auto n = VRPipeNode::create(e);
    nodes[cID] = n;
    nodesByName[name] = cID;
    return cID;
}

void VRPipeSystem::setDoVisual(bool b) { doVisual = b; }

PosePtr VRPipeSystem::getNodePose(int i) { return graph->getPosition(i); }
double VRPipeSystem::getSegmentPressure(int i) { return (segments[i]->pressure1+segments[i]->pressure2)*0.5; }
double VRPipeSystem::getSegmentFlow(int i) { return segments[i]->flow; }
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

    Vec3d dO = Vec3d(-0.1,-0.1,-0.1);
    Vec3d d1 = dO*2;

    if (rebuildMesh) {
        data.reset();
        rebuildMesh = false;
        Vec3d norm(0,1,0);
        Color3f white(1,1,1);
        Color3f yellow(1,1,0);
        Color3f blue(0.2,0.5,1);

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
            data.pushVert(p1->pos()+d1, norm, blue);
            data.pushVert(p2->pos()+d1, norm, blue);
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

        //cout << "apply data: " << data.size() << endl;
        data.apply(ptr());
    }

    // update system state

    int i=0;
    for (auto& s : segments) {
        //float pdelta = s.second->lastPressureDelta; // last written delta, not the correct one
        float pressure1 = s.second->pressure1;
        float pressure2 = s.second->pressure2;
        float density = s.second->density;
        float flow = s.second->flow;

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
        float t1 = 1.0 - pressure1; // around 1 bar
        float t2 = 1.0 - pressure2; // around 1 bar
        Color3f c1, c2;
        if (t1 > 0) c1 = Color3f(0, 0, 1-t1); // below 1 bar
        else c1 = Color3f(-t1*S, 0, 1+t1*S);
        if (t2 > 0) c2 = Color3f(0, 0, 1-t2); // below 1 bar
        else c2 = Color3f(-t2*S, 0, 1+t2*S);

        data.setColor(i, c1); i++;
        data.setColor(i, c2); i++;

        // density
        float D = 1.0; // color scale above 1
        Color3f cd;
        float d = 1.0 - density; // around 1
        if (d > 0) cd = Color3f(1-d, 1-d, 0); // below 1
        else cd = Color3f(1, 1, -d*D);

        data.setColor(i, cd); i++;
        data.setColor(i, cd); i++;


        // flow
        Color3f cf = Color3f(0,0,0);
        double f = min(1.0, abs(flow)*100.0);
        if (f > 1e-3) cf = Color3f(0.2,0.5*f,0.5+0.5*f);

        data.setColor(i, cf); i++;
        data.setColor(i, cf); i++;

        //cout << "flow " << s.first << " " << f << endl;
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

