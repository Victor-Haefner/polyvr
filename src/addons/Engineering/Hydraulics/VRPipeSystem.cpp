#include "VRPipeSystem.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "core/tools/VRAnnotationEngine.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/math/partitioning/graph.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRConcept.h"

using namespace OSG;

double gasSpeed = 300;


// Pipe End ----

VRPipeEnd::VRPipeEnd(VRPipeSegmentPtr s, int n, double h) { pipe = s; nID = n; offsetHeight = h; }
VRPipeEnd::~VRPipeEnd() {}
VRPipeEndPtr VRPipeEnd::create(VRPipeSegmentPtr s, int n, double h) { return VRPipeEndPtr( new VRPipeEnd(s,n,h) ); }


// Pipe Segment ----

VRPipeSegment::VRPipeSegment(int eID, double radius, double length, double level) : eID(eID), radius(radius), length(length) {
    pressurized = bool(level > 1.0-1e-6);
    setLevel(level);
    computeGeometry();
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(int eID, double radius, double length, double level) { return VRPipeSegmentPtr( new VRPipeSegment(eID, radius, length, level) ); }

VRPipeEndPtr VRPipeSegment::otherEnd(VRPipeEndPtr e) {
    bool isFirst = (end1.lock().get() == e.get());
    return isFirst ? end2.lock() : end1.lock();
}

void VRPipeSegment::setLength(double l) {
    length = l;
    computeGeometry();
}

void VRPipeSegment::computeGeometry() {
    area = Pi * pow(radius,2);
    volume = area * length;
    updateResistance();
}

void VRPipeSegment::setLevel(double lvl) {
    level = lvl;

    double h1 = fluidMin;
    double h2 = fluidMax;
    fluidLvl = min(h1, h2) + abs(h1 - h2) * level;
}

double VRPipeSegment::computeRegime(double Q) {
    // computeReynoldsNumber, inertial forces / viscous forces
    auto e1 = end1.lock();
    auto e2 = end2.lock();
    double D = 2*radius;
    double fill = max(level, 0.05); // avoid singularity
    double A = area * fill; // approximation of circular geometry
    if (A < 1e-9) return 1.0;

    double v = Q/A;
    double Re = density*v*D / viscosity;

    double k = clamp((Re - 2300) / (4000 - 2300), 0.0, 1.0); // normalize, <0 -> laminar, >1 -> turbulent
    //if (Q > 1e-3) cout << " -- computeRegime " << Q << ", " << v << ", " << Re << " -> " << k << endl;
    return k;
}

void VRPipeSegment::updateResistance() {
    double fill = max(level, 0.05);
    double rEff = radius * sqrt(fill);
    resistanceLaminar = (8.0 * viscosity * length) / (Pi * pow(rEff, 4)); // Hagen–Poiseuille resistance
    resistanceTurbulent = friction * length * density / ( 4 * radius * pow(area,2));
    //cout << " resistance: " << resistance << ", L: " << length << ", f: " << friction << ", D: " << density << ", R: " << radius << ", A: " << area << endl;
    if (resistanceLaminar < 1e-9) resistanceLaminar = 1.0;
    if (resistanceTurbulent < 1e-9) resistanceTurbulent = 1.0;
    resistanceLaminar = min(resistanceLaminar, 1e12); // TODO: rework this clamp
    resistanceTurbulent = min(resistanceTurbulent, 1e12); // TODO: rework this clamp
    //resistance = resistanceTurbulent; //max(resistanceLaminar, resistanceTurbulent); // TODO
}

double VRPipeSegment::computeEffectiveResistance(const double& flow) {
    double k = regime;
    double rho_g = density * gravity;
    double Rl = resistanceLaminar / rho_g;
    if (k <= 0.0) return max( Rl, 1e-9 );

    double Q = abs(flow);
    if (Q < 1e-6) return max( Rl, 1e-9 );
    double Rt = resistanceTurbulent * Q / rho_g;
    if (k >= 1.0) return max( Rt, 1e-9 );

    return max( Rl*(1.0-k) + Rt*k, 1e-9 );
};

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

    setupMaterial();
}

VRPipeSystem::~VRPipeSystem() {}

VRPipeSystemPtr VRPipeSystem::create() { return VRPipeSystemPtr( new VRPipeSystem() ); }
VRPipeSystemPtr VRPipeSystem::ptr() { return static_pointer_cast<VRPipeSystem>(shared_from_this()); }

void VRPipeSystem::setupMaterial() {
    auto m = VRMaterial::create("pipes");

    m->setLineWidth(5);
    m->setLit(0);
    m->setFrontBackModes(GL_NONE, GL_FILL);
    //m->setDepthTest(GL_ALWAYS);

    m->addPass();
    m->setPointSize(10);
    m->setLit(0);
    m->setFrontBackModes(GL_NONE, GL_FILL);
    //m->setDepthTest(GL_ALWAYS);

    setMaterial(m);
}

GraphPtr VRPipeSystem::getGraph() { return graph; }
VROntologyPtr VRPipeSystem::getOntology() { return ontology; }

VREntityPtr VRPipeSystem::getNodeEntity(int nID) {
    if (!nodes.count(nID)) return 0;
    return nodes[nID]->entity;
}

int VRPipeSystem::addNode(string name, PosePtr pos, string type, map<string, string> params) {
    rebuildMesh = true;
    auto e = ontology->addEntity(name, type);
    auto n = VRPipeNode::create(e);
    n->name = name;
    for (auto& p : params) e->set(p.first, p.second);
    int nID = graph->addNode(pos);
    n->nID = nID;
    nodes[nID] = n;
    nodesByName[name] = nID;
    return nID;
}

void VRPipeSystem::remNode(int nID) {
    if (!nodes.count(nID)) return;
    rebuildMesh = true;
    auto& node = nodes[nID];

    vector<int> eIDs;
    for (auto& e : node->pipes) {
        auto p = e->pipe.lock();
        if (p) eIDs.push_back(p->eID);
    }

    for (auto& eID : eIDs) remSegment(eID);

    ontology->remEntity(node->entity);
    nodesByName.erase(node->name);
    nodes.erase(nID);
    graph->remNode(nID);
}

int VRPipeSystem::getNode(string name) { return nodesByName[name]; }
string VRPipeSystem::getNodeName(int nID) { if (nodes.count(nID)) return nodes[nID]->name; return ""; }
int VRPipeSystem::getSegment(int n1, int n2) { return graph->getEdgeID(n1, n2); }

void VRPipeSystem::computeEndOffset(VRPipeEndPtr e) {
    auto entity = nodes[e->nID]->entity;

    if (entity->is_a("Tank")) {
        double H = entity->getValue("height", 1.0);
        Vec3d o = Vec3d(0,(e->offsetHeight-0.5)*H,0);
        e->offset = o;
    }

    auto pipe = e->pipe.lock();

    auto P = graph->getPosition(e->nID);
    e->height = (P->pos()+e->offset)[1];
}

void VRPipeSystem::computeHydraulicHead(VRPipeEndPtr e) { // initial guess
    e->hydraulicHead = e->height;// + (Ppipe - atmosphericPressure) / (pipeDensity * gravity);
}

int VRPipeSystem::addSegment(double radius, int n1, int n2, double level, double h1, double h2) {
    if (!nodes.count(n1)) return -1;
    if (!nodes.count(n2)) return -1;

    rebuildMesh = true;
    int sID = graph->connect(n1, n2);
    auto p1 = graph->getPosition(n1)->pos();
    auto p2 = graph->getPosition(n2)->pos();

    auto s = VRPipeSegment::create(sID, radius, 0.0, level);
    segments[sID] = s;
    auto e1 = VRPipeEnd::create(s, n1, h1);
    auto e2 = VRPipeEnd::create(s, n2, h2);
    s->end1 = e1;
    s->end2 = e2;

    nodes[n1]->pipes.push_back(e1);
    nodes[n2]->pipes.push_back(e2);
    computeEndOffset(e1);
    computeEndOffset(e2);
    computeHydraulicHead(e1);
    computeHydraulicHead(e2);

    p1[1] = e1->height;
    p2[1] = e2->height;
    Vec3d d = p2-p1;
    double length = d.length();
    s->setLength(length);

    double nz = (d/length)[1];
    double dz = radius * sqrt(1.0 - nz * nz);
    s->fluidMin = min(p1[1], p2[1]) - dz;
    s->fluidMax = max(p1[1], p2[1]) + dz;

    e1->height -= dz; // bottom of pipe
    e2->height -= dz; // bottom of pipe
    e1->heightMax = e1->height + 2*dz; // top of pipe
    e2->heightMax = e2->height + 2*dz; // top of pipe

    return sID;
}

void VRPipeSystem::remSegment(int eID) {
    if (!segments.count(eID)) return;
    rebuildMesh = true;
    auto& e = graph->getEdge(eID);
    graph->disconnect(e.from, e.to);
    auto& s = segments[eID];
    auto e1 = s->end1.lock();
    auto e2 = s->end2.lock();

    for (auto e : {e1,e2}) {
        if (!e) continue;
        if (!nodes.count(e->nID)) continue;
        auto node = nodes[e->nID];
        node->pipes.erase(remove(node->pipes.begin(), node->pipes.end(), e), node->pipes.end());
    }

    segments.erase(eID);
}

bool VRPipeSystem::goesIn(VRPipeSegmentPtr s, int nID) {
    auto edge = graph->getEdge(s->eID);
    return bool(edge.to == nID);
}

bool VRPipeSystem::goesOut(VRPipeSegmentPtr s, int nID) {
    auto edge = graph->getEdge(s->eID);
    return bool(edge.from == nID);
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

double VRPipeSystem::computeTotalMass() {
    double totalMass = 0.0;

    for (auto& n : nodes) { // mass in tanks
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 1.0);
            double tankHeight = entity->getValue("height", 1.0);
            double tankVolume = tankArea * tankHeight;
            double tankLevel = entity->getValue("level", 1.0); // 0..1
            double tankDensity = entity->getValue("density", waterDensity); // kg/m³
            totalMass += tankDensity * tankVolume * tankLevel;
        }
    }

    for (auto& s : segments) { // mass in pipes
        auto pipe = s.second;
        double pipeLevel = pipe->level; // 0..1
        double pipeDensity = pipe->density; // kg/m³
        double pipeVolume = pipe->volume; // m³
        totalMass += pipeDensity * pipeVolume * pipeLevel;
    }

    return totalMass;
}

void VRPipeSystem::initOntology() {
    ontology = VROntology::create("PipeSystem");
    auto Tank = ontology->addConcept("Tank");
    auto Pump = ontology->addConcept("Pump");
    auto Outlet = ontology->addConcept("Outlet");
    auto Junction = ontology->addConcept("Junction");
    auto Gauge = ontology->addConcept("Gauge");
    auto Valve = ontology->addConcept("Valve");
    auto CheckValve = ontology->addConcept("CheckValve", "Valve");
    auto ReliefValve = ontology->addConcept("ReliefValve", "Valve");
    auto ControlValve = ontology->addConcept("ControlValve", "Valve");
    auto ValvePath = ontology->addConcept("ValvePath");
    auto Cylinder = ontology->addConcept("Cylinder");

    Tank->addProperty("pressure", "double");
    Tank->addProperty("initialGasVolume", "double");
    Tank->addProperty("initialGasPressure", "double");
    Tank->addProperty("area", "double");
    Tank->addProperty("height", "double");
    Tank->addProperty("density", "double");
    Tank->addProperty("temperature", "double");
    Tank->addProperty("level", "double");
    Tank->addProperty("isOpen", "bool");
    Gauge->addProperty("pressure", "double");
    Gauge->addProperty("maxPressure", "double");
    Gauge->addProperty("tank", Tank);
    Pump->addProperty("maxHead", "double");
    Pump->addProperty("control", "double");
    Pump->addProperty("isOpen", "bool");
    Outlet->addProperty("radius", "double");
    Outlet->addProperty("pressure", "double");
    Outlet->addProperty("density", "double");
    Valve->addProperty("state", "double");
    CheckValve->addProperty("crackingPressure", "double");
    ReliefValve->addProperty("thresholdPressure", "double");
    ReliefValve->addProperty("reseatPressure", "double");
    ControlValve->addProperty("paths", ValvePath);
    Cylinder->addProperty("area", "double");
    Cylinder->addProperty("length", "double");
    Cylinder->addProperty("force", "double"); // external force
    Cylinder->addProperty("resistance", "double"); // external resistance
    Cylinder->addProperty("level1", "double");
    Cylinder->addProperty("level2", "double");
    Cylinder->addProperty("state", "double");
    Cylinder->addProperty("headFlow", "double");

    // (A,B,x0,xs,K) where A and B are ports and x0 the spool/state position, xs the transition size and K the resistance
    ValvePath->addProperty("A", "int");
    ValvePath->addProperty("B", "int");
    ValvePath->addProperty("x0", "double");
    ValvePath->addProperty("xs", "double");
    ValvePath->addProperty("K", "double");
}

void VRPipeSystem::addControlValvePath(int i, int A, int B, double x0, double xs, double K) {
    if (!nodes.count(i)) return;
    auto vp = ontology->addEntity("path", "ValvePath");
    vp->set("A", toString(A));
    vp->set("B", toString(B));
    vp->set("x0", toString(x0));
    vp->set("xs", toString(xs));
    vp->set("K", toString(K));
    auto& n = nodes[i];
    n->entity->add("paths", vp->getName());
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
    double lvl = segments[sID]->level;
    int cID = disconnect(nID, sID);
    addSegment(radius, nID, cID, lvl, 0, 0);
    return cID;
}

int VRPipeSystem::disconnect(int nID, int sID) {
    rebuildMesh = true;
    int cID = graph->split(nID, sID);
    auto e = ontology->addEntity("junction", "Junction");
    auto n = VRPipeNode::create(e);
    nodes[cID] = n;
    string name = e->getName();
    n->name = name;
    nodesByName[name] = cID;
    return cID;
}

void VRPipeSystem::setDoVisual(bool b, float s) {
    doVisual = b;
    spread = s;
    rebuildMesh = true;
    if (!b) updateInspection(0);
}

void VRPipeSystem::setTimeScale(double s) { timeScale = s; }

void VRPipeSystem::setVisuals(vector<string> ls) { layers = ls; }

VREntityPtr VRPipeSystem::getEntity(string name) {
    if (!nodesByName.count(name)) return 0;
    int nID = nodesByName[name];
    return nodes[nID]->entity;
}

PosePtr VRPipeSystem::getNodePose(int i) { return graph->getPosition(i); }
double VRPipeSystem::getPipeRadius(int i) { return segments[i]->radius; }
double VRPipeSystem::getSegmentPressure(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? (e1->pressure+e2->pressure)*0.5   : 0; }
Vec2d VRPipeSystem::getSegmentGradient(int i) {  auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->pressure, e2->pressure) : Vec2d(); }
Vec2d VRPipeSystem::getSegmentHead(int i) {  auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->hydraulicHead, e2->hydraulicHead) : Vec2d(); }
double VRPipeSystem::getSegmentDensity(int i) { return segments[i]->density; }
Vec2d VRPipeSystem::getSegmentFlow(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->flow, e2->flow) : Vec2d(); }
Vec2d VRPipeSystem::getSegmentHeadFlow(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->headFlow, e2->headFlow) : Vec2d(); }
double VRPipeSystem::getTankPressure(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("pressure", atmosphericPressure) : 0.0; }
double VRPipeSystem::getTankDensity(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("density", waterDensity) : 0.0; }
double VRPipeSystem::getTankLevel(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("level", 1.0) : 0.0; }
double VRPipeSystem::getPump(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("control", 0.0) : 0.0; }
double VRPipeSystem::getValveState(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("state", 1.0) : 1.0; }
void VRPipeSystem::setNodeCb(int i, VRAnimCbPtr cb) { if (!nodes.count(i)) return; nodes[i]->userCb = cb; }

void VRPipeSystem::setValve(int nID, double b)  { auto e = getNodeEntity(nID); if (e) e->set("state", toString(b)); }
void VRPipeSystem::setTankPressure(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("pressure", toString(p)); }
void VRPipeSystem::setTankDensity(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("density", toString(p)); }
void VRPipeSystem::setPipeRadius(int i, double r) { segments[i]->radius = r; segments[i]->computeGeometry(); }
void VRPipeSystem::setOutletDensity(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("density", toString(p)); }
void VRPipeSystem::setOutletPressure(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("pressure", toString(p)); }

void VRPipeSystem::setPipePressure(int i, double p1, double p2) {
    auto e1 = segments[i]->end1.lock();
    auto e2 = segments[i]->end2.lock();
    if (e1) e1->pressure = p1;
    if (e2) e2->pressure = p2;
}

void VRPipeSystem::setPump(int nID, double c, bool isOpen) {
    auto e = getNodeEntity(nID);
    if (e) {
        e->set("control", toString(c));
        e->set("isOpen", toString(isOpen));
    }
}

void VRPipeSystem::setFlowParameters(float l) {
    latency = l;
}

void VRPipeSystem::updateInspection(int nID) {
    if (!doVisual) {
        if (inspectionTool) {
            inspectionTool->clear();
            inspectionTool->destroy();
            inspectionTool = 0;
        }
        return;
    }

    if (!inspectionTool) {
        inspectionTool = VRAnalyticGeometry::create("inspectionTool");
        inspectionTool->setLabelParams(0.01, false, true);
        addChild(inspectionTool);
    }

    //if (inspected == nID) return;
    inspected = nID;
    if (!nodes.count(nID)) return;
    auto node = nodes[nID];
    if (!node) return;
    if (!node->entity) return;

    Vec3d d = Vec3d(-spread,-spread,-spread) * 0.2;
    auto P = getNodePose(nID);
    if (!P) return;
    Vec3d pn = getNodePose(nID)->pos();

    inspectionTool->clear();


    for (auto pIn : getInPipes(nID)) {
        auto e1 = pIn->end1.lock();
        auto e2 = pIn->end2.lock();
        double p1 = e1->pressure;
        double p2 = e2->pressure;
        double v = e1->flow;
        auto e = graph->getEdge(pIn->eID);
        Vec3d pn2 = getNodePose(e.from)->pos();
        Vec3d D = pn2-pn;
        inspectionTool->addVector(pn+d+D*0.05, D*0.45, Color3f(0,1,0), "p: " +toString(p1)+"->"+toString(p2));
        inspectionTool->addVector(pn+d+D*0.5 , D*0.45, Color3f(0,1,0), "v: " +toString(v));
    }


    for (auto pOut : getOutPipes(nID)) {
        auto e1 = pOut->end1.lock();
        auto e2 = pOut->end2.lock();
        double p1 = e1->pressure;
        double p2 = e2->pressure;
        double v = e1->flow;
        auto e = graph->getEdge(pOut->eID);
        Vec3d pn2 = getNodePose(e.to)->pos();
        Vec3d D = pn2-pn;
        inspectionTool->addVector(pn+d+D*0.05, D*0.45, Color3f(1,0,0), "p: " +toString(p1)+"->"+toString(p2));
        inspectionTool->addVector(pn+d+D*0.5 , D*0.45, Color3f(1,0,0), "v: " +toString(v));
    }

    auto labels = inspectionTool->getAnnotationEngine();
    if (!labels) return;
    string conceptName = "unknown";
    if (auto c = node->entity->getConcept() ) conceptName = c->getName();
    labels->add(pn, "Node "+toString(nID)+": "+conceptName);
}

void VRPipeSystem::updateVisual() {
    auto sign = [](double v) { return (v > 0) - (v < 0); };

    if (!doVisual) {
        VRGeoData data(ptr());
        if (data.size() > 0) {
            data.reset();
            data.apply(ptr());
        }
        return;
    }

    const Color3f white(1,1,1);
    const Color3f lgray(0.9,0.9,0.9);
    const Color3f yellow(1,1,0);
    const Color3f red(1,0,0);
    const Color3f blue(0.2,0.5,1);
    const Color3f dblue(0.1,0.4,0.7);
    const Color3f lblue(0.3,0.7,1);
    const Color3f green(0.2,1.0,0.2);

    VRGeoData data(ptr());

    Vec3d dO = Vec3d(-spread,-spread,-spread);

    auto mix = [](Color3f c1, Color3f c2, double t) {
        return c1+(c2-c1)*t;
    };

    auto getTempColor = [&](float T) -> Color3f {
        if (T <= 20) return mix(dblue, blue, T/20.0);
        if (T <= 100) return mix(blue, red, (T-20.0)/80.0);
        return red;
    };

    auto updatePipeInds = [&](VRGeoData& data, double level, int i0, int k0) {
        //cout << "updatePipeInds " << i0 << " " << k0 << endl;
        auto updateQuad = [&](VRGeoData& data, int k, int i0, int a, int b, int c, int d, Color3f col) {
            data.setIndex(k+0, i0+a);
            data.setIndex(k+1, i0+b);
            data.setIndex(k+2, i0+c);
            data.setIndex(k+3, i0+d);
            for (int i : {a,b,c,d}) data.setColor(i0+i, col);
        };

        Pnt3d p1 = data.getPosition(i0);
        Pnt3d p2 = data.getPosition(i0+1);
        Pnt3d p3 = data.getPosition(i0+12);
        Pnt3d p4 = data.getPosition(i0+13);
        Vec3d d = p3-p1; d.normalize();

        if (abs(d[1]) < 1e-6) { // horizontal
            updateQuad(data, k0+12*4, i0, 0, 0, 0, 0, dblue);
            updateQuad(data, k0+13*4, i0, 0, 0, 0, 0, dblue);

            updateQuad(data, k0+4*4, i0, 0, 3, 7, 6, blue); // sides bottom
            updateQuad(data, k0+5*4, i0, 12, 0, 6, 4, blue);
            updateQuad(data, k0+6*4, i0, 15, 12, 4, 5, blue);
            updateQuad(data, k0+7*4, i0, 3, 15, 5, 7, blue);

            updateQuad(data, k0+8*4, i0, 2, 1, 10, 11, white); // sides top
            updateQuad(data, k0+9*4, i0, 14, 2, 11, 9, white);
            updateQuad(data, k0+10*4, i0, 13, 14, 9, 8, white);
            updateQuad(data, k0+11*4, i0, 1, 13, 8, 10, white);

            updateQuad(data, k0+3*4, i0, 3, 0, 12, 15, dblue); // bottom
            updateQuad(data, k0+2*4, i0, 1, 2, 14, 13, white); // top
        } else if(abs(d[1]) > 1.0-1e-6) { // vertical
            updateQuad(data, k0+12*4, i0, 0, 0, 0, 0, dblue);
            updateQuad(data, k0+13*4, i0, 0, 0, 0, 0, dblue);

            updateQuad(data, k0+4*4, i0, 5, 7, 2, 3, blue); // sides bottom
            updateQuad(data, k0+5*4, i0, 7, 6, 1, 2, blue);
            updateQuad(data, k0+6*4, i0, 6, 4, 0, 1, blue);
            updateQuad(data, k0+7*4, i0, 4, 5, 3, 0, blue);

            updateQuad(data, k0+8*4, i0, 15, 14, 11, 9, white); // sides top
            updateQuad(data, k0+9*4, i0, 14, 13, 10, 11, white);
            updateQuad(data, k0+10*4, i0, 13, 12, 8, 10, white);
            updateQuad(data, k0+11*4, i0, 12, 15, 9, 8, white);

            updateQuad(data, k0+2*4, i0, 3, 2, 1, 0, dblue); // bottom
            updateQuad(data, k0+3*4, i0, 12, 13, 14, 15, white); // top
        } else { // inclined
            double hl = p1[1]+(p4[1]-p1[1])*level;
            std::array<double, 4> heights = { p1[1], p2[1], p3[1], p4[1] };
            bool cutFace1 = bool(hl < heights[1]);
            bool cutFace3 = bool(hl < heights[2]);

            bool face1Bellow = bool(hl >= heights[1]);
            bool face2Above  = bool(hl  < heights[1]);
            bool face3bellow = bool(hl >= heights[2]);
            bool face4Above  = bool(hl  < heights[2]);

            if (cutFace1) {
                updateQuad(data, k0+2*4, i0, 0, 3, 7, 6, blue); // bottom1
                updateQuad(data, k0+3*4, i0, 2, 1, 10, 11, white); // bottom2
                updateQuad(data, k0+4*4, i0, 1, 2, 14, 13, face2Above ? white : blue); // side
            } else { // cutFace2
                updateQuad(data, k0+2*4, i0, 3, 2, 1, 0, face1Bellow ? dblue : white); // bottom
                updateQuad(data, k0+3*4, i0, 1, 2, 7, 6, blue); // side1
                updateQuad(data, k0+4*4, i0, 14, 13, 10, 11, white); // side2
            }

            if (cutFace3) {
                updateQuad(data, k0+5*4, i0, 3, 0, 4, 5, blue); // side1
                updateQuad(data, k0+6*4, i0, 12, 15, 9, 8, white); // side2
                updateQuad(data, k0+7*4, i0, 12, 13, 14, 15, face4Above ? white : blue); // top
            } else { // cutFace4
                updateQuad(data, k0+5*4, i0, 3, 0, 12, 15, face3bellow ? blue : white); // side
                updateQuad(data, k0+6*4, i0, 15, 12, 4, 5, blue); // top1
                updateQuad(data, k0+7*4, i0, 13, 14, 9, 8, white); // top2
            }

            if (!cutFace1 && cutFace3) { // two sides cut
                updateQuad(data, k0+12*4, i0, 0, 0, 0, 0, white);
                updateQuad(data, k0+13*4, i0, 0, 0, 0, 0, white);
                updateQuad(data, k0+ 8*4, i0, 0, 1, 6, 4, blue);
                updateQuad(data, k0+ 9*4, i0, 13, 12, 8, 10, white);
                updateQuad(data, k0+10*4, i0, 2, 3, 5, 7, blue);
                updateQuad(data, k0+11*4, i0, 15, 14, 11, 9, white);
            }

            if (cutFace1 && !cutFace3) { // two caps cut
                updateQuad(data, k0+12*4, i0, 0, 0, 0, 0, white);
                updateQuad(data, k0+13*4, i0, 0, 0, 0, 0, white);
                updateQuad(data, k0+ 8*4, i0, 12, 0, 6, 4, blue);
                updateQuad(data, k0+ 9*4, i0, 1, 13, 8, 10, white);
                updateQuad(data, k0+10*4, i0, 3, 15, 5, 7, blue);
                updateQuad(data, k0+11*4, i0, 14, 2, 11, 9, white);
            }

            if (cutFace1 && cutFace3) { // level low
                updateQuad(data, k0+ 8*4, i0, 0, 0, 6, 4, blue);
                updateQuad(data, k0+ 9*4, i0, 13, 12, 8, 10, white);
                updateQuad(data, k0+12*4, i0, 10, 1, 13, 13, white);
                updateQuad(data, k0+10*4, i0, 3, 3, 5, 7, blue);
                updateQuad(data, k0+11*4, i0, 15, 14, 11, 9, white);
                updateQuad(data, k0+13*4, i0, 2, 11, 14, 14, white);
            }

            if (!cutFace1 && !cutFace3) { // level high
                updateQuad(data, k0+ 8*4, i0, 0, 1, 6, 4, blue);
                updateQuad(data, k0+12*4, i0, 0, 0, 4, 12, blue);
                updateQuad(data, k0+ 9*4, i0, 13, 13, 8, 10, white);
                updateQuad(data, k0+10*4, i0, 2, 3, 5, 7, blue);
                updateQuad(data, k0+13*4, i0, 3, 3, 15, 5, blue);
                updateQuad(data, k0+11*4, i0, 14, 14, 11, 9, white);
            }

            data.setColor(i0+0, dblue);
            data.setColor(i0+3, dblue);
        }
    };

    if (rebuildMesh) {
        //cout << "updateVisual - rebuildMesh" << endl;
        auto ann = dynamic_pointer_cast<VRAnnotationEngine>( findFirst("testInds") );
        if (!ann) {
            ann = VRAnnotationEngine::create("testInds");
            ann->setSize(0.03);
            ann->setBillboard(true);
            ann->getMaterial()->setDepthTest(GL_ALWAYS);
            addChild(ann);
        }

        data.reset();
        rebuildMesh = false;
        Vec3d norm(0,1,0);

        for (auto& s : segments) {
            s.second->lastVizLevel = -1.0;
            auto edge = graph->getEdge(s.first);
            double r = s.second->radius;
            double l = s.second->length;

            Vec3d p1 = graph->getPosition(edge.from)->pos();
            Vec3d p2 = graph->getPosition(edge.to)->pos();

            auto e1 = s.second->end1.lock();
            auto e2 = s.second->end2.lock();
            p1 += e1->offset;
            p2 += e2->offset;
            Vec3d pm = (p1+p2)*0.5;

            Vec3d dPipe = (p2-p1); dPipe.normalize();

            Vec2d tcID1 = Vec2d(edge.from, 0);
            Vec2d tcID2 = Vec2d(edge.to, 0);

            Color3f col1 = white;
            Color3f col2 = white;
            int k = 0;
            for (auto l : layers) {
                if (l == "p") { col1 = white; col2 = white; }
                else if (l == "d") { col1 = yellow; col2 = yellow; }
                else if (l == "v") { col1 = blue; col2 = blue; }
                else if (l == "n") { col1 = white; col2 = green; }
                else if (l == "h") { col1 = green; col2 = green; }
                else continue;

                data.pushVert(p1+dO*k, norm, col1, tcID1);
                if (l == "h") { data.pushVert(pm+dO*k, norm, col1, tcID1); data.pushLine(); }
                data.pushVert(p2+dO*k, norm, col2, tcID2);
                data.pushLine();
                k++;
            }

            // pipe ends
            data.pushVert(p1+dPipe*l*0.02, norm, e1->pressurized ? dblue : white, Vec2d());
            data.pushPoint();
            data.pushVert(p2-dPipe*l*0.02, norm, e2->pressurized ? dblue : white, Vec2d());
            data.pushPoint();

            // water box
            if (p2[1] < p1[1]) swap(p1,p2); // p2 always higher than p1
            pm = (p1+p2)*0.5;
            Vec3d d = (p2-p1); d.normalize();

            double g = r*2;
            Vec3d u = Vec3d(1,0,0);
            if (d[1] < 1.0-1e-6) { u = d.cross(Vec3d(0,1,0)); u.normalize(); }
            double a = d.enclosedAngle(Vec3d(0,1,0));
            double t = g*cos(a) + l*sin(a);
            int i0 = data.size();
            int k0 = data.getNIndices();

            data.pushQuad(p1, d, u, Vec2d(g,g), false);
            data.pushQuad(pm, Vec3d(0,1,0), u, Vec2d(t,g), true);
            data.pushQuad(pm, Vec3d(0,1,0), u, Vec2d(t,g), false);
            data.pushQuad(p2, d, u, Vec2d(g,g), false);
            for (int i=0; i<4; i++) data.pushColor(dblue);
            for (int i=0; i<4; i++) data.pushColor(blue);
            for (int i=0; i<4; i++) data.pushColor(white);
            for (int i=0; i<4; i++) data.pushColor(white);

            data.pushQuad(-9, -10, -11, -12); // mid but reversed
            for (int i=0; i<12; i++) data.pushQuad(-1,-1,-1,-1); // placeholders
            updatePipeInds(data, 0.5, i0, k0);

            // flow visual
            Vec3d u2 = d.cross(u) + u; u2.normalize();
            float g2 = g*1.05;
            float g3 = g*0.5;
            float g4 = g2/sqrt(2);
            data.pushQuad(pm, d, u,  Vec2d(g3,g2), false);
            data.pushQuad(pm, d, u,  Vec2d(g2,g3), false);
            data.pushQuad(pm, d, u2, Vec2d(g4,g4), false);
            for (int i=0; i<12; i++) data.pushColor(green);
            for (int i=0; i<4; i++) {
                int a = 0;
                int b = 1;
                if (i == 0) { a = 0; b = 1; }
                if (i == 1) { a = 5; b = 6; }
                if (i == 2) { a = 2; b = 3; }
                if (i == 3) { a = 7; b = 4; }

                int i0 = -12;
                int i1 = -8;
                int i2 = -4;
                data.pushQuad(i2+i,i2+i,i0+a,i0+b);
                data.pushQuad(i2+i,i2+i,i0+b,i0+a);
                data.setNorm(i1+i, dPipe); // store dPipe
                data.setNorm(i2+i, Vec3d(data.getPosition(i2+i))); // store p0
            }
        }

        auto createWaterBox = [&](Vec3d p, double a, double b, double h) {
            data.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), false);
            data.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), true);
            data.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), false);
            data.pushQuad(p + Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), true);
            data.pushQuad(-13, -14, -15, -16); // bottom
            data.pushQuad(-9, -10, -11, -12); // mid but reversed

            data.pushQuad(-9, -10, -14,  -13); // sides bottom
            data.pushQuad(-10, -11, -15, -14);
            data.pushQuad(-11, -12, -16, -15);
            data.pushQuad(-12, -9,  -13, -16);

            data.pushQuad(-1, -2, -6,  -5); // sides top
            data.pushQuad(-2, -3, -7, -6);
            data.pushQuad(-3, -4, -8, -7);
            data.pushQuad(-4, -1,  -5, -8);
            for (int i=0; i<4; i++) data.pushColor(dblue);
            for (int i=0; i<4; i++) data.pushColor(blue);
            for (int i=0; i<4; i++) data.pushColor(white);
            for (int i=0; i<4; i++) data.pushColor(white);
        };

        for (auto& n : nodes) {
            auto p = graph->getPosition(n.first);
            auto e = n.second->entity;

            if (e->is_a("Tank")) {
                double a = e->getValue("area", 1.0);
                double h = e->getValue("height", 1.0);
                double s = sqrt(a);
                createWaterBox(p->pos(), s, s, h);
                continue;
            }

            if (e->is_a("Cylinder")) {
                double a = e->getValue("area", 1.0);
                double l = e->getValue("length", 1.0);
                double h = sqrt(a);
                double s = l*0.5;
                createWaterBox(p->pos()-Vec3d(s*0.5,0,0), s, h, h);
                createWaterBox(p->pos()+Vec3d(s*0.5,0,0), s, h, h);
                continue;
            }

            data.pushVert(p->pos(), norm, white, Vec2d());
            data.pushPoint();
        }

        //cout << "apply data: " << data.size() << endl;
        data.apply(ptr());
    }

    // update system state

    int i = 0;
    int k = 0;
    static double F = 0; F += 0.01; // TOTEST

    auto ann = dynamic_pointer_cast<VRAnnotationEngine>( findFirst("testInds") );
    for (auto& s : segments) {
        auto e1 = s.second->end1.lock();
        auto e2 = s.second->end2.lock();
        //float pdelta = s.second->lastPressureDelta; // last written delta, not the correct one
        float pressure1 = e1->pressure;
        float pressure2 = e2->pressure;
        float density = s.second->density;
        float flow = e1->flow;

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

        // density
        float D = 1.0; // color scale above 1
        Color3f cd;
        float d = 1.0 - density; // around 1
        if (d > 0) cd = Color3f(1-d, 1-d, 0); // below 1
        else cd = Color3f(1, 1, -d*D);

        // flow
        Color3f cf = Color3f(0,0,0);
        double f = min(1.0, abs(flow)*100.0);
        if (f > 1e-3) cf = Color3f(0.3*f,0.7*f,0.5+0.5*f);

        // hydraulic head
        double h1 = e1->hydraulicHead;
        double h2 = s.second->hydraulicHead;
        double h3 = e2->hydraulicHead;

        // visuals
        Color3f col1, col2;
        for (auto l : layers) {
            if (l == "p") { col1 = c1; col2 = c2; }
            else if (l == "d") { col1 = cd; col2 = cd; }
            else if (l == "v") { col1 = cf; col2 = cf; }
            else if (l == "n") { col1 = white; col2 = green; }
            else if (l == "h") {
                col1 = green; col2 = green;
                auto p1 = data.getPosition(i)  ; p1[1] = h1; data.setPos(i  , p1);
                auto p2 = data.getPosition(i+1); p2[1] = h2; data.setPos(i+1, p2);
                auto p3 = data.getPosition(i+2); p3[1] = h3; data.setPos(i+2, p3);
            } else continue;

            data.setColor(i, col1); i++;
            data.setColor(i, col2); i++;
            if (l == "h") { i++; k += 2; }
            k += 2;
        }

        data.setColor(i, e1->pressurized ? dblue : lgray); i++;
        data.setColor(i, e2->pressurized ? dblue : lgray); i++;
        k += 2;

        double l = s.second->level;
        //l = 0.5+0.5*sin(F); // TOTEST

        l = clamp(l, 1e-3, 1.0-1e-3);
        if (abs(l-s.second->lastVizLevel) > 1e-3) {
            //cout << "updateVisual - update segment level" << endl;
            s.second->lastVizLevel = l;

            std::array<double, 4> heights;
            heights[0] = data.getPosition(i+ 0)[1];
            heights[1] = data.getPosition(i+ 2)[1];
            heights[2] = data.getPosition(i+12)[1];
            heights[3] = data.getPosition(i+14)[1];
            std::sort(heights.begin(), heights.end());
            double h = heights[0] + l*(heights[3] - heights[0]);

            vector<Pnt3d> intersections;
            auto intersectHz = [&](int a, int b) { // intersect edge (a,b) with horizontal plane at height h
                Pnt3d p1 = data.getPosition(i+a);
                Pnt3d p2 = data.getPosition(i+b);
                if (abs(p2[1]-p1[1]) < 1e-6) return;
                double k = (h-p1[1])/(p2[1]-p1[1]);
                if (k < 1e-6 || k > 1.0-1e-6) return;
                Pnt3d I = p1 + k*(p2-p1);
                intersections.push_back(I);
                //cout << " (" << a << "," << b << "," << k << ")";
            };

            //cout << " I ";
            // order important!
            intersectHz(0, 12);
            intersectHz(12, 13);
            intersectHz(3, 15);
            intersectHz(15, 14);
            intersectHz(1, 13);
            intersectHz(2, 14);
            intersectHz(0, 1);
            intersectHz(3, 2);
            //cout << endl;

            if (intersections.size() == 4) {
                for (int j=0; j<4; j++) {
                    data.setPos(i+4+j, intersections[j]);
                    data.setPos(i+8+j, intersections[j]);
                }
            }
            updatePipeInds(data, l, i, k);

            /*for (int j=0; j<4; j++) {
                if (!s.second->pressurized) data.setColor(i+4+j, green);
                else data.setColor(i+4+j, blue);
            }*/
        }

        auto tmpCol1 = getTempColor(e1->temperature);
        auto tmpCol2 = getTempColor(e2->temperature);
        //if (e1->temperature > 21 || e2->temperature > 21) cout << "T1 " << e1->temperature << ", T2 " << e2->temperature << endl;
        //auto tmpCol1 = getTempColor(90.0);
        //auto tmpCol2 = getTempColor(90.0);

        tmpCol1 = s.second->pressurized ? blue : lblue;
        tmpCol2 = tmpCol1;

        data.setColor(i+4+0, tmpCol1);
        data.setColor(i+4+1, tmpCol1);
        data.setColor(i+4+2, tmpCol2);
        data.setColor(i+4+3, tmpCol2);


        double r = s.second->radius;
        double v0 = 0.5; // m/s
        double v_ref = 4.0; // m/s
        double v = flow/s.second->area;
        double F = -sign(v) * 2.0*r*std::asinh(abs(v) / v0) / std::asinh(v_ref / v0);

        for (int j=0; j<4; j++) {
            int j1 = i+20+j;
            int j2 = i+24+j;
            Vec3d sD = data.getNormal(j1);
            Vec3d p0 = data.getNormal(j2);
            data.setPos(j2, p0 + F*sD);
        }

        /*for (int j=0; j<16; j++) {
            Vec3d p = Vec3d(data.getPosition(i+j));
            p += Vec3d(j, j, j)*0.0005;
            ann->set(j, p, toString(j));
        }*/

        // level + flow
        i += 16 + 12;
        k += 56 + 32;
    }

    auto updateWaterBox = [&](double lvl, Color3f col) {
        for (int j=0; j<4; j++) {
            Pnt3d p = data.getPosition(i+j);
            p[1] += lvl;
            data.setPos(i+4+j, p);
            data.setPos(i+8+j, p);
            data.setColor(i+4+j, col);
        }

        i += 16;
        k += 48;
    };

    for (auto& n : nodes) {
        auto e = n.second->entity;
        if (e->is_a("Tank")) {
            double T = e->getValue("temperature", 20.0);
            auto col = getTempColor(T);

            double l = e->getValue("level", 1.0);
            double h = e->getValue("height", 1.0);
            updateWaterBox(h*l, col);
            continue;
        }

        if (e->is_a("Cylinder")) {
            double state = e->getValue("state", 1.0);
            double L = e->getValue("length", 1.0);
            Pnt3d p1 = data.getPosition(i+1)    + Vec3d(L*state,0,0);
            Pnt3d p2 = data.getPosition(i+2)    + Vec3d(L*state,0,0);
            Pnt3d p3 = data.getPosition(i+12+1) + Vec3d(L*state,0,0);
            Pnt3d p4 = data.getPosition(i+12+2) + Vec3d(L*state,0,0);
            data.setPos(i+0,    p1);
            data.setPos(i+3,    p2);
            data.setPos(i+12+0, p3);
            data.setPos(i+12+3, p4);
            data.setPos(i+16+1, p1);
            data.setPos(i+16+2, p2);
            data.setPos(i+28+1, p3);
            data.setPos(i+28+2, p4);

            double l1 = e->getValue("level1", 1.0);
            double l2 = e->getValue("level2", 1.0);
            double a = e->getValue("area", 1.0);
            double h = sqrt(a);
            updateWaterBox(l1*h, blue);
            updateWaterBox(l2*h, blue);
            continue;
        }

        Color3f c(0.4,0.4,0.4);
        if (e->is_a("ControlValve")) {
            ; // TODO
        }
        else if (e->is_a("Valve")) {
            double s = e->getValue("state", 0.0);
            c = s>0.9 ? Color3f(0,1,0) : Color3f(1,0,0);
        }

        if (e->is_a("Junction")) c = Color3f(0.2,0.4,1);
        if (e->is_a("Outlet"))   c = Color3f(0.1,0.2,0.8);

        if (e->is_a("Pump")) {
            double p = e->getValue("control", 0.0);
            c = p>1e-3 ? Color3f(1,1,0) : Color3f(1,0.5,0);
        }

        data.setColor(i, c); i++;
        k++;
    }
}



/** ---- simulation ---- */

double VRPipeSystem::clamp(double x, double a, double b, bool warn, string lbl) {
    if (warn) {
        double eps = 1e-9;
        if (x > b+eps) cout << "Warning in VRPipeSystem::clamp " << lbl << ": " << x << " > " << b << ", d: " << abs(x-b) << endl;
        if (x < a-eps) cout << "Warning in VRPipeSystem::clamp " << lbl << ": " << x << " < " << a << ", d: " << abs(x-b) << endl;
    }
    return x<a ? a : x>b ? b : x;
}

void VRPipeSystem::assignBoundaryPressures() {
    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        auto nPos = graph->getPosition(n.first)->pos();

        if (entity->is_a("Outlet")) {
            double outletRadius = entity->getValue("radius", 0.0);
            if (outletRadius < 1e-9) continue; // closed

            double outletPressure = entity->getValue("pressure", atmosphericPressure);
            for (auto& e : node->pipes) {
                auto pipe = e->pipe.lock();
                e->hydraulicHead = e->height + outletPressure/pipe->density/gravity;
            }
            continue;
        }

        if (entity->is_a("Tank")) {
            double tankPressure = entity->getValue("pressure", atmosphericPressure);
            double tankDensity = entity->getValue("density", waterDensity);
            double tankHeight = entity->getValue("height", 1.0);
            double tankLevel = entity->getValue("level", 1.0);
            bool tankOpen = entity->getValue("isOpen", false);
            if (tankOpen) tankPressure = atmosphericPressure;

            for (auto& e : node->pipes) {
                double fluidHeight = (tankLevel-0.5)*tankHeight + nPos[1];
                double depth = max(0.0, fluidHeight - e->height);
                double Pfluid = depth * tankDensity * gravity;
                double Pgauge = tankPressure + Pfluid - atmosphericPressure;
                e->hydraulicHead = e->height + Pgauge / (tankDensity * gravity);
                //cout << " tank boundary expr.: hH: " << e->hydraulicHead << ", h: " << e->height << ", d: " << depth << endl;
            }
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];

            double l1 = entity->getValue("level1", 1.0);
            double l2 = entity->getValue("level2", 1.0);
            double a = entity->getValue("area", 1.0);
            double h = sqrt(a);
            double h1 = nPos[1] + h*(l1-0.5);
            double h2 = nPos[1] + h*(l2-0.5);
            if (!e1->pressurized) e1->hydraulicHead = h1;
            if (!e2->pressurized) e2->hydraulicHead = h2;
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        if (pipe->pressurized && e1->pressurized && e2->pressurized) continue;

        if (!pipe->pressurized) { pipe->hydraulicHead = pipe->fluidLvl; continue; }
        pipe->hydraulicHead = max(pipe->hydraulicHead, pipe->fluidLvl);
    }
}

void VRPipeSystem::solveNodeHeads(double dt) {
    auto simpleAverage = [&](const vector<VRPipeEndPtr>& ends) -> double {
        if (ends.size() == 0) return 0;
        if (ends.size() == 1) return ends[0]->pipe.lock()->hydraulicHead;

        double H = 0;
        for (auto& e : ends) {
            auto pipe = e->pipe.lock();
            H += pipe->hydraulicHead;
        }
        return H / ends.size();
    };

    auto computeAverage = [&](const vector<VRPipeEndPtr>& ends) -> double {
        if (ends.size() == 0) return 0;
        if (ends.size() == 1) return ends[0]->pipe.lock()->hydraulicHead;

        double num = 0.0;
        double den = 0.0;

        //cout << "computeAverage ";

        for (auto& e : ends) {
            auto pipe = e->pipe.lock();
            double R = pipe->computeEffectiveResistance(e->flow);
            double H = pipe->hydraulicHead;
            num += H / R;
            den += 1.0 / R;
            //cout << " H/R: " << H << "/" << R;
        }
        //cout << " -> new head: " << num / den << endl;

        if (abs(den) < 1e-13) return simpleAverage(ends);
        return num / den;
    };

    auto averageOverPipes = [&](vector<VRPipeEndPtr> ends, double& maxHeadDelta) {
        double newHead = computeAverage(ends);

        for (auto& e : ends) {
            maxHeadDelta = max(maxHeadDelta, abs(e->hydraulicHead-newHead));
            e->hydraulicHead = newHead;
        }
    };

    auto processPumpHeads = [&](vector<VRPipeEndPtr> ends, VREntityPtr entity, double& maxHeadDelta) -> bool {
        if (ends.size() != 2) return false;

        double control = entity->getValue("control", 0.0);
        double maxHead = entity->getValue("maxHead", 0.0);
        bool isOpen = entity->getValue("isOpen", 0.0);
        auto pEnd1 = ends[0];
        auto pEnd2 = ends[1];

        if (control < 1e-3) { // pump off
            if (!isOpen) {
                averageOverPipes({pEnd1}, maxHeadDelta);
                averageOverPipes({pEnd2}, maxHeadDelta);
                return true;
            } else return false;
        }

        averageOverPipes({pEnd1}, maxHeadDelta);
        averageOverPipes({pEnd2}, maxHeadDelta);

        double deltaHead = pEnd2->hydraulicHead - pEnd1->hydraulicHead;
        double pumpGain = control * maxHead;
        double mod = clamp(pumpGain - deltaHead, 0.0, pumpGain);
        maxHeadDelta = max(maxHeadDelta, abs(mod));

        pEnd1->hydraulicHead -= mod;
        pEnd2->hydraulicHead += mod;
        //cout << " processPumpHeads " << mod << ", " << pEnd1->hydraulicHead << ", " << pEnd2->hydraulicHead << ", " << pumpGain << ", " << maxHead << endl;
        return true;
    };

    auto processValve = [&](vector<VRPipeEndPtr> ends, VREntityPtr entity, double& maxHeadDelta) -> bool {
        if (ends.size() != 2) return false;
        double x = entity->getValue("state", 0.0);
        if (x < 1e-3) {
            averageOverPipes({ends[0]}, maxHeadDelta);
            averageOverPipes({ends[1]}, maxHeadDelta);
        } else averageOverPipes({ends[0], ends[1]}, maxHeadDelta);
        return true;
    };

    auto processCylinder = [&](vector<VRPipeEndPtr> ends, VREntityPtr entity, double& maxHeadDelta) -> bool {
        if (ends.size() != 2) return false;
        auto& e1 = ends[0];
        auto& e2 = ends[1];

        if (!e1->pressurized && !e2->pressurized) return true;
        if (e1->pressurized) averageOverPipes({e1}, maxHeadDelta);
        if (e2->pressurized) averageOverPipes({e2}, maxHeadDelta);

        // create step
        double dh = e2->hydraulicHead - e1->hydraulicHead;
        e1->hydraulicHead += dh*0.01;
        e2->hydraulicHead -= dh*0.01;

        return true;
    };

    auto processControlValve = [&](VRPipeNodePtr node, VREntityPtr entity, double& maxHeadDelta) -> bool {
        vector<VRPipeEndPtr>& ends = node->pipes;
        auto paths = entity->getAllEntities("paths");
        double x = entity->getValue("state", 0.0);

        auto& endGroup = node->endGroup;
        auto& pathOpenings = node->pathOpenings;
        endGroup.clear();
        pathOpenings.clear();

        for (auto& p : paths) {
            int A = p->getValue("A", -1);
            int B = p->getValue("B", -1);
            double x0 = p->getValue("x0", 0.0);
            double xs = p->getValue("xs", 0.0);
            double K  = p->getValue("K" , 0.0);
            int Ne = (int)ends.size();

            if (A<0 || B<0 || A>=Ne || B>=Ne) {
                cout << "Error in processControlValve, wrong ends indices: " << A << ", " << B << endl;
                continue;
            }

            double y = K * max(0.0, 1.0-abs(x-x0)/xs); // tent function around x0

            if (y > 1e-3) { // connected
                if (endGroup.count(A)) endGroup[B] = endGroup[A];
                else if (endGroup.count(B)) endGroup[A] = endGroup[B];
                else {
                    int g = endGroup.size();
                    endGroup[A] = g;
                    endGroup[B] = g;
                }

                if (!pathOpenings.count(A)) pathOpenings[A] = 0;
                if (!pathOpenings.count(B)) pathOpenings[B] = 0;
                pathOpenings[A] += y;
                pathOpenings[B] += y;
            }
        }

        for (size_t i=0; i<ends.size(); i++) { // handle ends without paths/groups
            if (!endGroup.count(i)) averageOverPipes({ends[i]}, maxHeadDelta);
        }

        map<int, vector<int>> groups;
        for (auto& eg : endGroup) {
            if (!groups.count(eg.second)) groups[eg.second] = vector<int>();
            groups[eg.second].push_back(eg.first);
        }

        for (auto& g : groups) {
            vector<VRPipeEndPtr> gEnds;
            for (auto& e : g.second) gEnds.push_back(ends[e]);
            averageOverPipes(gEnds, maxHeadDelta);
        }
        return true;
    };

    double maxHeadDelta = 1; // convergence criteria
    double eps = 0.01;
    for (int i=0; maxHeadDelta>eps && i<50; i++) {
        maxHeadDelta = 0;

        for (auto& n : nodes) { // update pipe-end heads for each node
            auto node = n.second;
            auto entity = node->entity;
            if (entity->is_a("Tank") || entity->is_a("Outlet")) continue; // already prescribed
            if (entity->is_a("Pump") && processPumpHeads(node->pipes, entity, maxHeadDelta)) continue;
            if (entity->is_a("ControlValve") && processControlValve(node, entity, maxHeadDelta)) continue;
            else if (entity->is_a("Valve") && processValve(node->pipes, entity, maxHeadDelta)) continue;
            if (entity->is_a("Cylinder") && processCylinder(node->pipes, entity, maxHeadDelta)) continue;
            averageOverPipes(node->pipes, maxHeadDelta);
        }

        for (auto& s : segments) { // update head for each pipe
            auto& pipe = s.second;
            auto e1 = pipe->end1.lock();
            auto e2 = pipe->end2.lock();

            double newHead = pipe->fluidLvl;
            if (pipe->pressurized) {
                if (e1->pressurized && e2->pressurized) newHead = (e1->hydraulicHead + e2->hydraulicHead) * 0.5;
                else if (e1->pressurized) newHead = (e1->hydraulicHead + pipe->fluidLvl) * 0.5;
                else if (e2->pressurized) newHead = (e2->hydraulicHead + pipe->fluidLvl) * 0.5;
            }

            maxHeadDelta = max(maxHeadDelta, abs(pipe->hydraulicHead-newHead));
            pipe->hydraulicHead = newHead;
        }
    }
}

void VRPipeSystem::computeHeadFlows(double dt) {
    auto sign = [](double v) { return (v > 0) - (v < 0); };

    auto computeLaminarFlow = [&](const double& dP, const VRPipeSegmentPtr& pipe, bool halfLength) -> double {
        double R = pipe->resistanceLaminar;
        if (R < 1e-12) return 0.0;
        if (halfLength) R *= 0.5;
        return dP / R;
    };

    auto computeTurbulentFlow = [&](const double& dP, const VRPipeSegmentPtr& pipe, bool halfLength) -> double {
        double R = pipe->resistanceTurbulent;
        if (R < 1e-12) return 0.0;
        if (halfLength) R *= 0.5;
        int dir = sign(dP);
        return sqrt( abs(dP) / R ) * dir;
    };

    auto computeFlow = [&](const double& dH, const VRPipeSegmentPtr& pipe, bool halfLength = false) -> double {
        double dP = dH * pipe->density * gravity;

        auto computeFlowByRegime = [&](double k) {
            double Q = 0;
            if (k <= 0.0) Q = computeLaminarFlow(dP, pipe, halfLength);
            else if (k >= 1.0) Q = computeTurbulentFlow(dP, pipe, halfLength);
            else {
                double Ql = computeLaminarFlow(dP, pipe, halfLength);
                double Qt = computeTurbulentFlow(dP, pipe, halfLength);
                Q = Ql*(1.0-k) + Qt*k;
            }
            return Q;
        };

        double k = pipe->regime;
        double Q = computeFlowByRegime(k);
        double k2 = pipe->computeRegime(abs(Q));
        if (abs(k2-k) > 0.1) Q = computeFlowByRegime(k2); // avoid numeric worst cases, big regime switches!

        //if (abs(Q)> 1e-3 && Q < 0.0) cout << " computeFlow " << " Q: " << Q << ", k: " << k << ", k2: " << k2 << ", dH: " << dH << ", dP: " << dP << endl;
        return Q;
    };

    auto accellerateFlow = [&](const double& dH, const VRPipeSegmentPtr& pipe, const double& Q, bool halfLength = false) -> double {
        double dP = dH * pipe->density * gravity;
        double Aeff = pipe->area * max(pipe->level, 0.01);
        double L = pipe->density * pipe->length / max(Aeff, 1e-9); // inertance
        double R = pipe->computeEffectiveResistance(Q);
        if (halfLength) { R *= 0.5; L *= 0.5; }
        double loss = R * Q * pipe->density * gravity;

        double dQ = (dP-loss) / L;
        double maxStep = 2.0 * abs(dP) / L;
        dQ = clamp(dQ, -maxStep, maxStep); // stability

        return Q + dQ*dt;
    };

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();

        if (pipe->pressurized && e1->pressurized && e2->pressurized) {
            double dH = e2->hydraulicHead - e1->hydraulicHead; // compute hydraulic gradient
            double flow = accellerateFlow(dH, pipe, e1->flow);

            e1->headFlow =  flow;
            e2->headFlow = -flow;
        } else {
            for (auto& e : {e1,e2}) {
                double dH = e->hydraulicHead - pipe->hydraulicHead;
                double flow = computeFlow(dH, pipe, true);
                e->headFlow = -flow;
                //if (abs(flow) > 1e-3) cout << " --- end " << e << ", flow: " << e->headFlow << endl;
            }
        }
    }

    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];

            // compute piston movement and flow
            double Fext = entity->getValue("force", 0.0);
            double R = entity->getValue("resistance", 0.1);
            double x = entity->getValue("state", 0.0);
            double L = entity->getValue("length", 0.0);
            double a = entity->getValue("area", 0.0);

            double p1 = e1->hydraulicHead;
            double p2 = e2->hydraulicHead;

            double Fhyd = -(p2 - p1) * a;
            double v = (Fhyd - Fext) / max(R, 1e-9);

            double dx = v * dt;
            double x_new = clamp(x + dx/L, 0.001, 0.999);
            dx = (x_new - x) * L;
            double hflow = a*dx / dt;
            entity->set("headFlow", toString(hflow));
            //cout << " cylinder flow: " << hflow << endl;
        }
    }
}

void VRPipeSystem::computeMaxFlows(double dt) {
    //cout << "computeMaxFlows" << endl;
    auto computeContainerFlowScaling = [&](string ID, double volAir, double volWater, double flowIn, double flowOut, bool pressurized) {
        double totalFlow = flowIn - flowOut;
        double volDelta  = totalFlow*dt;
        double maxFlowIn  = flowIn;
        double maxFlowOut = flowOut;

        if ( volDelta > volAir) maxFlowIn = min(flowIn,  volAir/dt + flowOut); // node will fill up, limit flow in

        if (-volDelta > volWater && pressurized) { // node will drain, limit flow out
            maxFlowOut =  min(flowOut, volWater/dt + flowIn);
        }

        if ( flowOut*dt > volWater && !pressurized) {
            maxFlowOut =  min(flowOut, volWater/dt);
        }

        double scaleFlowIn  = abs(flowIn ) > 1e-12 ? maxFlowIn  / flowIn  : 1.0;
        double scaleFlowOut = abs(flowOut) > 1e-12 ? maxFlowOut / flowOut : 1.0;

        return Vec2d(scaleFlowIn, scaleFlowOut);
    };

    auto clampCylinderFlows = [&](VRPipeEndPtr& e, double cVolume, double cLevel, double pistonFlow) {
        // pistonFlow < 0: piston makes chamber smaller
        // pistonFlow > 0: piston makes chamber bigger

        double vAir = cVolume * (1.0-cLevel);
        double vWat = cVolume * cLevel;

        double flow = e->maxFlow;
        // flow < 0: water leaves the chamber
        // flow > 0: water enters the chamber

        double deltaWaterVol = flow*dt;
        double deltaPistonVol = pistonFlow*dt;

        double cVolume2 = cVolume + deltaPistonVol;
        double vWat2 = vWat + deltaWaterVol;

        if (flow < 0.0 && vWat2 < 0.0) { // too much flow out
            double maxDeltaVolOut = vWat;
            flow = -min(-flow, maxDeltaVolOut/dt);
        }

        if (flow > 0.0 && vWat2 > cVolume2) { // too much flow in
            double maxDeltaVolIn = cVolume2 - vWat;
            flow =  min( flow, maxDeltaVolIn/dt);
        }
        deltaWaterVol = flow*dt;

        if (e->pressurized) {
            if (pistonFlow > 0 && flow > 0) {
                pistonFlow = min(flow, pistonFlow);
                flow = min(flow, pistonFlow);
            }
        }

        // test too much push out
        double maxPistonFlow = (vAir - deltaWaterVol)/dt;
        if (-pistonFlow > maxPistonFlow) pistonFlow = -maxPistonFlow;

        //cout << " clampCylinderFlows fc: " << pistonFlow << ", newLevel: " << newLevel << ", newVolWater: " << newVolWater << ", newVol: " << newVol << endl;
        //if (hflow > 0 && e1->pressurized) hflow = min(flow1, hflow);
        //if (hflow < 0 && e2->pressurized) hflow =-min(flow2,-hflow);

        e->maxFlow = flow;
        return pistonFlow;
    };

    auto processNodes = [&]() {
        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;

            if (entity->is_a("Cylinder")) {
                if (n.second->pipes.size() != 2) continue;

                double cArea = entity->getValue("area", 1.0);
                double cLength = entity->getValue("length", 1.0);
                double cVolume = cArea * cLength;
                double cLevel1 = entity->getValue("level1", 1.0);
                double cLevel2 = entity->getValue("level2", 1.0);
                double hflow = entity->getValue("headFlow", 0.0);
                double cState = entity->getValue("state", 0.0);
                double cVol1 = cVolume * cState;
                double cVol2 = cVolume * (1.0-cState);

                auto& e1 = n.second->pipes[0];
                auto& e2 = n.second->pipes[1];

                for (int i=0; i<2; i++) {
                    hflow =  clampCylinderFlows(e1, cVol1, cLevel1,  hflow);
                    hflow = -clampCylinderFlows(e2, cVol2, cLevel2, -hflow);
                }

                //cout << " maxflow wV: " << cVol1*cLevel1 << " -> " << cVol1*cLevel1 + hflow*dt << ", " << (cVol1*cLevel1 + hflow*dt)/(cVol1+hflow*dt) << endl;
                //cout << " cylinder cV1 " << cVol1 << ", cV: " << cVolume << ", x: " << cState << endl;
                //cout << "  flows: " << e1->maxFlow << ", " << hflow << ", " << e2->maxFlow << endl;

                //hflow = min( abs(flow2), min(abs(flow1), abs(hflow)) ) * (hflow < 0 ? -1 : 1);
                entity->set("headFlow", toString(hflow));

            } else {
                double nodeAirVolume = 0.0;
                double nodeWaterVolume = 0.0;

                if (entity->is_a("Tank")) {
                    double tankArea = entity->getValue("area", 0.0);
                    double tankHeight = entity->getValue("height", 0.0);
                    double tankVolume = tankHeight * tankArea;
                    double tankLevel = entity->getValue("level", 1.0);
                    nodeAirVolume = tankVolume * (1.0-tankLevel);
                    nodeWaterVolume = tankVolume * tankLevel;
                }

                double totalFlowIn = 0;
                double totalFlowOut = 0;
                for (auto& e : n.second->pipes) {
                    auto f = e->maxFlow;
                    if (f < 0) totalFlowOut += -f;
                    else totalFlowIn += f;
                }

                Vec2d scaleFlowInOut = computeContainerFlowScaling(node->name, nodeAirVolume, nodeWaterVolume, totalFlowIn, totalFlowOut, true);

                for (auto& e : n.second->pipes) {
                    auto f = e->maxFlow;
                    if (f < 0) e->maxFlow = f * scaleFlowInOut[1];
                    else e->maxFlow = f * scaleFlowInOut[0];
                }
            }
        }
    };

    auto processSegments = [&](bool& needsIteration) {
        //cout << "check pipes max flow" << endl;
        for (auto& s : segments) {
            auto& pipe = s.second;
            auto e1 = pipe->end1.lock();
            auto e2 = pipe->end2.lock();

            double pipeAirVolume = pipe->volume * (1.0-pipe->level);
            double pipeWaterVolume = pipe->volume * pipe->level;

            double totalFlowIn = 0;
            double totalFlowOut = 0;
            for (auto& e : {e1, e2}) {
                auto f = e->maxFlow;
                if (f < 0) totalFlowIn += -f;
                else totalFlowOut += f;
            }

            //if ((totalFlowIn - totalFlowOut)*dt > pipeWaterVolume)
            //if (totalFlowOut*dt > pipeWaterVolume && !pipe->pressurized)
            //    cout << " flow limited, flow: " << totalFlowIn - totalFlowOut << ", water vol: " << pipeWaterVolume << ", dt: " << dt << endl;

            Vec2d scaleFlowInOut = computeContainerFlowScaling("seg"+toString(pipe->eID), pipeAirVolume, pipeWaterVolume, totalFlowIn, totalFlowOut, pipe->pressurized);
            if (scaleFlowInOut[0] < 0.9) needsIteration = true;
            if (scaleFlowInOut[1] < 0.9) needsIteration = true;

            for (auto& e : {e1, e2}) {
                auto f = e->maxFlow;
                if (f < 0) e->maxFlow = f * scaleFlowInOut[0];
                else e->maxFlow = f * scaleFlowInOut[1];
            }

            // forbid cavitations
            if (e1->pressurized && e2->pressurized) {
                double totalFlowIn = 0;
                double totalFlowOut = 0;
                for (auto& e : {e1, e2}) {
                    auto f = e->maxFlow;
                    if (f < 0) totalFlowIn += -f;
                    else totalFlowOut += f;
                }

                if (totalFlowOut > totalFlowIn) {
                    for (auto& e : {e1, e2}) {
                        auto f = e->maxFlow;
                        if (f > 0) e->maxFlow = totalFlowIn;
                    }
                    needsIteration = true;
                }
            }
        }
    };

    auto computeMaxValveFlow = [&](double state, double A, double dH) { // TODO, doesnt work
        double a = state * A;
        double Qmax = a * sqrt(2.0 * gravity * abs(dH));
        return Qmax;
        //cout << "i: " << i << ", h " << e->headFlow << ", Qm " << Qmax << ", f " << e->maxFlow << ", state: " << state << endl;
    };

    auto copyInitialMaxHead = [&]() {
        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;

            for (size_t i=0; i<node->pipes.size(); i++) {
                auto& e = node->pipes[i];

                if (entity->is_a("Tank")) {
                    double tankLevel = entity->getValue("level", 1.0);
                    double tankHeight = entity->getValue("height", 1.0);
                    auto nPos = graph->getPosition(n.first)->pos();
                    double fluidHeight = (tankLevel-0.5)*tankHeight + nPos[1];
                    if (e->height > fluidHeight && e->headFlow < 0) {
                        e->maxFlow = 0;
                        continue; // pipe end above water level cant drain tank!
                    }
                }

                if (entity->is_a("ControlValve")) {
                    auto pipe = e->pipe.lock();
                    if (!pipe) continue;
                    if (node->pathOpenings.count(i)) {
                        double state = node->pathOpenings[i];
                        /*double dH = e->hydraulicHead - pipe->otherEnd(e)->hydraulicHead;
                        double Qmax = computeMaxValveFlow(state, pipe->area, dH);
                        e->maxFlow = clamp(e->headFlow, -Qmax, Qmax);*/
                        e->maxFlow = e->headFlow * state;
                        continue;
                    }
                }

                else if (entity->is_a("Valve")) {
                    auto pipe = e->pipe.lock();
                    if (!pipe) continue;

                    double state = entity->getValue("state", 0.0);
                    /*double dH = e->hydraulicHead - pipe->otherEnd(e)->hydraulicHead;
                    double Qmax = computeMaxValveFlow(state, pipe->area, dH);
                    e->maxFlow = clamp(e->headFlow, -Qmax, Qmax);*/
                    e->maxFlow = e->headFlow * state;
                    continue;
                }

                e->maxFlow = e->headFlow;
            }
        }
    };

    auto copyFinalMaxHead = [&]() {
        for (auto& n : nodes) {
            for (auto& e : n.second->pipes) {
                e->flow = e->maxFlow;
            }
        }
    };


    copyInitialMaxHead();

    bool needsIteration = true;
    //cout << "itr max flow.." << endl;
    for (int i=0; needsIteration && i<30; i++) {
        needsIteration = false;
        processNodes();
        processSegments(needsIteration);
    }

    copyFinalMaxHead();
}

void VRPipeSystem::updateLevels(double dt) { // TODO: split updatePressurized from updateLevels
    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

        for (auto& pEnd : node->pipes) pEnd->pressurized = true;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 0.0);
            double tankHeight = entity->getValue("height", 0.0);
            double tankVolume = tankHeight * tankArea;
            double tankLevel = entity->getValue("level", 1.0);
            bool tankOpen = entity->getValue("isOpen", false);

            double totalFlow = 0;
            for (auto& pEnd : node->pipes) {
                auto pipe = pEnd->pipe.lock();
                if (!pipe) continue;
                totalFlow += pEnd->flow; // positive flow is defined as going out the pipe
            }

            double volDelta = totalFlow*dt;
            double newLevel = clamp(tankLevel + volDelta / tankVolume, 0.0, 1.0, true, "tank lvl");
            volDelta = (newLevel - tankLevel)*tankVolume;
            entity->set("level", toString(newLevel));

            // update pressurized tank
            if (!tankOpen) {
                double oldVolume = (1.0-tankLevel)*tankVolume;
                double tankPressure = entity->getValue("pressure", atmosphericPressure);
                double initialGasVolume = entity->getValue("initialGasVolume", oldVolume); // avoid drift
                double initialGasPressure = entity->getValue("initialGasPressure", tankPressure); // avoid drift

                double newVolume = (1.0-newLevel) *tankVolume;
                double p = initialGasPressure * initialGasVolume / newVolume;
                //cout << " tank: " << n.first << " p gauge: " << tankPressure-atmosphericPressure << " level: " << newLevel << " tV " << tankVolume << endl;

                p = clamp(p, atmosphericPressure * 0.001, atmosphericPressure * 2000.0, true, "tank pressure");
                entity->set("pressure", toString(p));
                //cout << " new tank pressure: " << p << endl;
            }

            // update pipes not submerged by fluid level
            auto nPos = graph->getPosition(n.first)->pos();
            double fluidHeight = (newLevel-0.5)*tankHeight + nPos[1];
            for (auto& e : node->pipes) {
                if (e->height > fluidHeight) e->pressurized = false;
            }
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];

            // compute piston movement
            double x = entity->getValue("state", 0.0);
            double L = entity->getValue("length", 0.0);
            double a = entity->getValue("area", 0.0);
            double dflow = entity->getValue("headFlow", 0.0);
            double vol = L*a;
            double dx = dflow * dt / vol;

            double x_new = dx + x;
            entity->set("state", toString(x_new));

            // compute new levels
            double _lvl1 = entity->getValue("level1", 1.0);
            double _lvl2 = entity->getValue("level2", 1.0);
            double _vol1 = vol * x;
            double _vol2 = vol * (1.0-x);
            double _volWater1 = _vol1*_lvl1;
            double _volWater2 = _vol2*_lvl2;

            double vol1 = vol * x_new;
            double vol2 = vol * (1.0-x_new);
            double volWater1 = clamp(_volWater1 + e1->flow*dt, 0.0, vol1, true, "cylinder volWater1");
            double volWater2 = clamp(_volWater2 + e2->flow*dt, 0.0, vol2, true, "cylinder volWater2");
            double lvl1 = volWater1/vol1;
            double lvl2 = volWater2/vol2;

            /*cout << " level cV: " << vol << ", x: " << x << " -> " << x_new << endl;
            cout << " level flows f1: " << e1->flow << ", fc: " << dflow << ", f2: " << e2->flow << endl;
            cout << " level dVw1: " << e1->flow*dt << ", dVc: " << dflow*dt << ", dVw2: " << e2->flow*dt << endl;
            cout << " level V1: " << _vol1 << " -> " << vol1 << ", V2: " << _vol2 << " -> " << vol2 << endl;
            cout << " level wV1: " << _volWater1 << " -> " << volWater1 << ", wV2: " << _volWater2 << " -> " << volWater2 << endl;
            cout << " level wV1+cf: " << _volWater1 << " -> " << _volWater1+dflow*dt << ", wV2-cf: " << _volWater2 << " -> " << _volWater2-dflow*dt << endl;
            cout << " level lvl1: " << _lvl1 << " -> " << lvl1 << ",  lvl2: " << _lvl2 << " -> " << lvl2 << endl;
            cout << " level lvl1-1: " << _lvl1-1.0 << " -> " << lvl1-1.0 << ",  lvl2-1: " << _lvl2-1.0 << " -> " << lvl2-1.0 << endl;
            cout << " level max dflow: " << (_vol2*(1.0-_lvl2) - e2->flow*dt)/dt << ", dflow: " << dflow << endl;*/
            //cout << "    " << volWater2 / vol2 - 1.0 << " " << vol2 - volWater2 << endl;

            double newLevel1 = clamp(lvl1, 0.0, 1.0, true, "cylinder lvl1");
            double newLevel2 = clamp(lvl2, 0.0, 1.0, true, "cylinder lvl2");

            entity->set("level1", toString(newLevel1));
            entity->set("level2", toString(newLevel2));


            // update pressurized
            if (newLevel1 < 0.999) e1->pressurized = false;
            if (newLevel2 < 0.999) e2->pressurized = false;

            if (n.second->userCb) {
                (*n.second->userCb)(x_new);
            }
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        double flow = e1->flow + e2->flow; // positive flow is going out the pipe
        double lvl = clamp(pipe->level - flow*dt / pipe->volume, 0.0, 1.0, true);
        pipe->setLevel(lvl);

        // hysteresis
        if ( pipe->pressurized && pipe->level < 0.95) pipe->pressurized = false;
        if (!pipe->pressurized && pipe->level > 0.98) pipe->pressurized = true;
        //pipe->pressurized = bool(pipe->level > 1.0-1e-6);
        //pipe->pressurized = true;

        if (!pipe->pressurized) {
            //if (pipe->eID == 24) cout << " -- pipe24, fLvl: " << pipe->fluidLvl << ", fluidMin: " << pipe->fluidMin;
            for (auto& e : {e1,e2}) {
                //if (pipe->eID == 24) cout << ", end ht: " << e->heightMax;
                if (e->heightMax > pipe->fluidLvl) e->pressurized = false;
                //if (pipe->eID == 24) cout << " pr: " << e->pressurized;
            }
            //if (pipe->eID == 24) cout << endl;
        }
    }

    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (entity->is_a("Tank")) continue;
        if (entity->is_a("Cylinder")) continue;

        if (entity->is_a("ControlValve")) { // only unpressurize if connecting path

            vector<VRPipeEndPtr>& ends = node->pipes;
            auto paths = entity->getAllEntities("paths");
            double x = entity->getValue("state", 0.0);

            auto& endGroup = node->endGroup;
            map<int, vector<int>> groups;
            for (auto& eg : endGroup) {
                if (!groups.count(eg.second)) groups[eg.second] = vector<int>();
                groups[eg.second].push_back(eg.first);
            }

            for (auto& g : groups) {
                vector<VRPipeEndPtr> gEnds;
                for (auto& e : g.second) gEnds.push_back(ends[e]);
                bool isP = true;
                for (auto& e : gEnds) if (!e->pressurized) isP = false;
                if (!isP) for (auto& e : gEnds) e->pressurized = false;
            }
            continue;
        }

        if (entity->is_a("Valve")) { // only unpressurize if state > 1e-3
            double x = entity->getValue("state", 0.0);
            if (x >= 1e-3) {
                bool isP = true;
                for (auto& e : node->pipes) if (!e->pressurized) isP = false;
                if (!isP) for (auto& e : node->pipes) e->pressurized = false;
            }
            continue;
        }

        bool isP = true;
        for (auto& e : node->pipes) if (!e->pressurized) isP = false;
        if (!isP) for (auto& e : node->pipes) e->pressurized = false;
    }
}

void VRPipeSystem::updatePressures(double dt) {
    for (auto& n : nodes) {
        for (auto& e : n.second->pipes) {
            auto pipe = e->pipe.lock();
            e->pressure = e->hydraulicHead * pipe->density * gravity;
        }

        if (n.second->userCb) {
            auto e = n.second->entity;
            if (e && e->is_a("Gauge")) {
                auto t = e->getEntity("tank");
                double maxPressure = e->getValue("maxPressure", 10*atmosphericPressure);
                double pressure = e->getValue("pressure", 0.0) - atmosphericPressure;
                if (t && maxPressure > 1e-3) {
                    double pt = t->getValue("pressure", 0.0) - atmosphericPressure;
                    double indicator = pt/maxPressure;
                    if (abs(pt - pressure)>1e-3) {
                        //cout << "updatePressures gauge " << pt << "/" << maxPressure << ", " << indicator << endl;
                        e->set("pressure", toString(pt));
                        (*n.second->userCb)(indicator);
                    }
                }
            }
        }
    }
}

void VRPipeSystem::updateRegimes(double dt) {
    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        double Q = max(abs(e1->flow), abs(e2->flow)); // pick active side
        pipe->regime = pipe->computeRegime(Q);
        pipe->updateResistance();
   }
}

void VRPipeSystem::computeAdvectiveHeatTransfer(double dt) {
    for (auto& n : nodes) { // --- 1. Reset heat flux accumulator at nodes ---
        auto e = n.second->entity;
        for (auto pe : n.second->pipes) {
            pe->heatFlux = 0.0;   // temporary accumulator (add this member)

            if (e->is_a("Tank")) {
                double T = e->getValue("temperature", 20.0);
                pe->temperature = T;
            }
        }
    }

    for (auto& s : segments) { // --- 2. Compute advective heat transport through pipes ---
        auto& pipe = s.second;
        auto end1 = pipe->end1.lock();
        auto end2 = pipe->end2.lock();

        double Q = end1->flow;
        // Convention: end1->flow = flow OUT of end1
        // So Q > 0 means flow end1 → end2

        if (Q < 0.0) { // comes inside the pipe from e1, goes to e2 (TODO: only if pipe is pressurized!)
            // Flow from end1 to end2
            double transportedHeat = abs(Q * end1->temperature);
            end1->heatFlux -= transportedHeat;  // leaving
            end2->heatFlux += transportedHeat;  // entering
        } else { // Flow from end2 to end1
            double transportedHeat = abs(Q * end2->temperature);
            end2->heatFlux -= transportedHeat;
            end1->heatFlux += transportedHeat;
        }
    }

    //double thermalCapacity = 1.0; // for later
    double thermalCapacity = 0.001; // for later

    for (auto& n : nodes) { // --- 3. Update node temperatures ---
        double heatFlux = 0.0;
        double meanTemp = 0.0;

        for (auto pe : n.second->pipes) {
            heatFlux += pe->heatFlux;
            meanTemp += pe->temperature;
        }

        meanTemp /= n.second->pipes.size();

        double dT = (dt / thermalCapacity) * heatFlux;
        meanTemp += dT;

        for (auto pe : n.second->pipes) {
            pe->temperature = meanTemp;
            //if (pe->temperature > 21) cout << n.second->nID << ", T " << pe->temperature << ", dT " << dT << endl;
        }
    }
}

void VRPipeSystem::update() {
    int subSteps = 10;
    double dT = 1.0/60;
    dT *= timeScale;
    double dt = dT/subSteps;

    //sleep(1);


    for (int i=0; i<subSteps; i++) {
        assignBoundaryPressures();
        //auto t1 = VRTimer::create();
        solveNodeHeads(dt);
        //auto T1 = t1->stop();
        computeHeadFlows(dt);
        //auto t2 = VRTimer::create();
        computeMaxFlows(dt); // most time spent
        //auto T2 = t2->stop();
        updateLevels(dt);
        computeAdvectiveHeatTransfer(dt);
        updatePressures(dt);
        updateRegimes(dt);
        //cout << " VRPipeSystem::update " << T1 << ", " << T2 << endl;
    }

    updateVisual();

    //cout << "mass " << computeTotalMass() << endl;
}

void VRPipeSystem::printSystem() {
    cout << "system:" << endl;
    for (auto n : nodes) {
        cout << " node " << n.second->name << endl;
        for (auto e : n.second->pipes) {
            cout << "  end head " << e->hydraulicHead << endl;
        }
    }
}
