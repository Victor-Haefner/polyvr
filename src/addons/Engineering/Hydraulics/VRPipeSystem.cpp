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

// fluid composition

void VRFluidComposition::mixIn(const VRFluidComposition& fluid, const double& percentage) {
    double k = percentage;
    double _k = 1.0-k;

    auto mixLin = [&](double& a, double b) {
        double m = a * _k + b * k;
        m = clamp(m, min(a, b), max(a,b));
        a = m;
    };

    auto mixLog = [&](double& a, double b) {
        double m = exp( log(a) * _k + log(b) * k );
        m = clamp(m, min(a, b), max(a,b));
        a = m;
    };

    mixLin(temperature, fluid.temperature);
    mixLin(baseDensity, fluid.baseDensity);
    mixLog(baseViscosity, fluid.baseViscosity);

    for (auto& p : fluid.particles) {
        if (!particles.count( p.first )) {
            particles[p.first] = p.second;
            particles[p.first].volumeFraction = 0.0;
        }
        mixLin(particles[p.first].volumeFraction, p.second.volumeFraction);
    }

    for (auto& p : particles) {
        if (fluid.particles.count( p.first )) continue;
        mixLin(particles[p.first].volumeFraction, 0.0);
    }

    updateThermalDependencies();
}

void VRFluidComposition::fromEntity(VREntityPtr e) {
    temperature = e->getValue("temperature", 20.0);
    baseDensity = e->getValue("density", 1000.0);
    baseViscosity = e->getValue("viscosity", 1e-3);

    map<string, VREntityPtr> eParts;
    for (auto& pe : e->getAllEntities("particles")) {
        string t = pe->getValue<string>("type", "");
        eParts[t] = pe;
        auto te = pe->getStringValue("type").p;
    }

    for (auto& p : particles) p.second.volumeFraction = 0.0;

    for (auto& pe : eParts) {
        string type = pe.first;

        if (!particles.count(type)) {
            particles[type] = ParticleBin();
            particles[type].ID = type;
        }

        auto& pb = particles[type];
        pb.sizeMin = pe.second->getValue("sizeMin", 0.0);
        pb.sizeMax = pe.second->getValue("sizeMax", 0.0);
        pb.density = pe.second->getValue("density", 1.0);
        pb.volumeFraction = pe.second->getValue("volumeFraction", 0.0);
    }

    updateThermalDependencies();
}

bool VRFluidComposition::toEntity(VREntityPtr e) {
    bool addedBin = false;
    e->set("temperature", toString(temperature, 12));
    e->set("density", toString(baseDensity, 12));
    e->set("viscosity", toString(baseViscosity, 12));
    auto ontology = e->getOntology();

    map<string, VREntityPtr> eParts;
    for (auto& pe : e->getAllEntities("particles")) {
        string t = pe->getValue<string>("type", "");
        eParts[t] = pe;
        pe->set("volumeFraction", "0.0");
    }

    for (auto& p : particles) {
        string type = p.second.ID;

        VREntityPtr pe = 0;
        if (!eParts.count(type)) {
            pe = ontology->addEntity(type, "ParticleBin");
            e->add("particles", pe->getName());
            addedBin = true;
        } else pe = eParts[type];

        pe->set("type", type);
        pe->set("sizeMin", toString(p.second.sizeMin));
        pe->set("sizeMax", toString(p.second.sizeMax));
        pe->set("density", toString(p.second.density, 12));
        pe->set("volumeFraction", toString(p.second.volumeFraction, 12));
    }
    return addedBin;
}

double VRFluidComposition::computeParticlesMass(double Volume) {
    double m = 0;
    for (auto& p : particles) {
        m += p.second.density * p.second.volumeFraction * Volume;
    }
    return m;
}

double VRFluidComposition::computeFluidMass(double Volume) {
    double pMass = computeParticlesMass(Volume);
    return effectiveDensity * Volume - pMass;
}

void VRFluidComposition::updateThermalDependencies() {
    double dT = temperature - 20.0;
    double rhoFluid = baseDensity - 0.3*dT;
    double muFluid  = baseViscosity * exp(-0.025*dT); // rough generic temp correction

    // particle/slurry mixture
    double phi = 0.0;
    double rhoParticles = 0.0;
    for (auto& p : particles) {
        double vf = clamp(p.second.volumeFraction, 0.0, 0.95);
        phi += vf;
        rhoParticles += vf * p.second.density;
    }
    phi = clamp(phi, 0.0, 0.95);

    effectiveDensity   = rhoFluid * (1.0 - phi) + rhoParticles;
    effectiveDensity   = max(effectiveDensity,   1.0);
    effectiveViscosity = muFluid * (1.0 + 2.5*phi);
    effectiveViscosity = max(effectiveViscosity, 1e-6);
}

double VRPipeSystem::getTankParticles(int nID, string type) {
    auto e = getNodeEntity(nID);
    if (!e) return 0.0;
    e = e->getEntity("fluid");
    if (!e) return 0.0;
    for (auto& pe : e->getAllEntities("particles")) {
        string t = pe->getValue<string>("type", "");
        if (type == t) return pe->getValue("volumeFraction", 0.0);
    }
    return 0.0;
}

void VRPipeSystem::setTankParticles(int nID, string type, double volFrac) {
    auto e = getNodeEntity(nID);
    if (!e) return;
    e = e->getEntity("fluid");
    if (!e) return;
    for (auto& pe : e->getAllEntities("particles")) {
        string t = pe->getValue<string>("type", "");
        if (type == t) {
            pe->set("volumeFraction", toString(volFrac, 12));
            return;
        }
    }
}

double VRPipeSystem::removeTankParticles(int nID, string type, double part) {
    auto e = getNodeEntity(nID);
    if (!e) return 0.0;
    auto fe = e->getEntity("fluid");
    if (!fe) return 0.0;

    double tankArea = e->getValue("area", 1.0);
    double tankHeight = e->getValue("height", 1.0);
    double tankVolume = tankHeight * tankArea;
    double tankLevel = e->getValue("level", 1.0);
    double fluidVolume = tankVolume * tankLevel;
    if (tankLevel < 1e-3) return 0.0;

    VREntityPtr pe = 0;
    for (auto& p : fe->getAllEntities("particles")) {
        string t = p->getValue<string>("type", "");
        if (type == t) { pe = p; break; }
    }
    if (!pe) return 0.0;

    double vF = pe->getValue("volumeFraction", 0.0);
    double d = pe->getValue("density", 1.0);
    double volOld = fluidVolume * vF;
    double volDelta = volOld * part;
    volDelta = clamp(volDelta, 0.0, fluidVolume, true, "tank particles volume");
    double mass = d * volDelta;

    fluidVolume -= volDelta;
    double volFrac = (volOld - volDelta) / fluidVolume;
    pe->set("volumeFraction", toString(volFrac, 12));

    // update tank level
    tankLevel = fluidVolume / tankVolume;
    e->set("level", toString(tankLevel,12));
    return mass;
}

void VRPipeSystem::addTankParticles(int nID, string type, double mass) {
    auto e = getNodeEntity(nID);
    if (!e) return;
    auto fe = e->getEntity("fluid");
    if (!fe) return;

    double tankArea = e->getValue("area", 1.0);
    double tankHeight = e->getValue("height", 1.0);
    double tankVolume = tankHeight * tankArea;
    double tankLevel = e->getValue("level", 1.0);
    double fluidVolume = tankVolume * tankLevel;
    if (tankLevel < 1e-3) return;

    VREntityPtr pe = 0;
    for (auto& p : fe->getAllEntities("particles")) {
        string t = p->getValue<string>("type", "");
        if (type == t) { pe = p; break; }
    }
    if (!pe) return;

    double d = pe->getValue("density", 1.0);
    double volDelta = mass / d;
    double volOld = fluidVolume * pe->getValue("volumeFraction", 0.0);
    volDelta = clamp(volDelta, 0.0, tankVolume-fluidVolume, true, "tank particles volume");
    fluidVolume += volDelta;

    double volFrac = (volOld + volDelta) / fluidVolume;
    pe->set("volumeFraction", toString(volFrac, 12));

    // update tank level
    double newLevel = fluidVolume / tankVolume;
    e->set("level", toString(newLevel,12));

    bool tankOpen = e->getValue("isOpen", false);
    if (!tankOpen) { // update pressurized tank
        double oldVolume = (1.0-tankLevel)*tankVolume;
        double tankPressure = e->getValue("pressure", atmosphericPressure);
        double initialGasVolume = e->getValue("initialGasVolume", oldVolume); // avoid drift
        double initialGasPressure = e->getValue("initialGasPressure", tankPressure); // avoid drift

        double newVolume = (1.0-newLevel) *tankVolume;
        double p = initialGasPressure * initialGasVolume / newVolume;
        //cout << " tank: " << n.first << " p gauge: " << tankPressure-atmosphericPressure << " level: " << newLevel << " tV " << tankVolume << endl;

        p = clamp(p, atmosphericPressure * 0.001, atmosphericPressure * 2000.0, false, "tank pressure");
        e->set("pressure", toString(p, 12));
        //cout << " new tank pressure: " << p << endl;
    }
}

void VRPipeSystem::addTankParticleBin(int nID, string type, Vec2d sizeRange, double density) {
    auto e = getNodeEntity(nID);
    if (!e) return;
    e = e->getEntity("fluid");
    if (!e) return;

    VREntityPtr be = 0;
    for (auto& pe : e->getAllEntities("particles")) {
        string t = pe->getValue<string>("type", "");
        if (type == t) { be = pe; break; }
    }

    if (!be) { // add new bin
        be = ontology->addEntity(type, "ParticleBin");
        be->set("type", type);
        e->add("particles", be->getName());
        rebuildMesh = true;
    }

    be->set("sizeMin", toString(sizeRange[0]));
    be->set("sizeMax", toString(sizeRange[1]));
    be->set("density", toString(density,  12));
    be->set("volumeFraction", "0.0");
}

// Pipe End ----

VRPipeEnd::VRPipeEnd(VRPipeSegmentPtr s, int n, double h) { pipe = s; nID = n; offsetHeight = h; }
VRPipeEnd::~VRPipeEnd() {}
VRPipeEndPtr VRPipeEnd::create(VRPipeSegmentPtr s, int n, double h) { return VRPipeEndPtr( new VRPipeEnd(s,n,h) ); }

void VRPipeEnd::updateGeometry(GraphPtr graph) {
    auto s = pipe.lock();
    auto p = graph->getPosition(nID)->pos();
    height = p[1] + offset[1] - s->zRadius; // bottom of pipe
    heightMax = height + 2*s->zRadius; // top of pipe
}

void VRPipeEnd::setInitialHead() { // initial guess
    hydraulicHead = height;// + (Ppipe - atmosphericPressure) / (pipeDensity * gravity);
}

// Pipe Segment ----

VRPipeSegment::VRPipeSegment(int eID, double radius, double length, double level) : eID(eID), radius(radius), length(length) {
    pressurized = bool(level > 1.0-1e-6);
    setLevel(level);
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(int eID, double radius, double length, double level) { return VRPipeSegmentPtr( new VRPipeSegment(eID, radius, length, level) ); }

VRPipeEndPtr VRPipeSegment::otherEnd(VRPipeEndPtr e) {
    bool isFirst = (end1.lock().get() == e.get());
    return isFirst ? end2.lock() : end1.lock();
}

void VRPipeSegment::setLevel(double lvl) {
    level = lvl;
    double h1 = fluidMin;
    double h2 = fluidMax;
    fluidLvl = min(h1, h2) + abs(h1 - h2) * level;
}

void VRPipeSegment::updateGeometry(GraphPtr graph, double friction) {
    auto e1 = end1.lock();
    auto e2 = end2.lock();
    auto p1 = graph->getPosition(e1->nID)->pos();
    auto p2 = graph->getPosition(e2->nID)->pos();

    p1[1] += e1->offset[1];
    p2[1] += e2->offset[1];
    Vec3d d = p2-p1;

    length = d.length();
    area = Pi * pow(radius,2);
    volume = area * length;

    updateResistance(friction);

    if (length < 1e-6) {
        zRadius = radius;
    } else {
        double nz = (d/length)[1];
        zRadius = radius * sqrt(1.0 - nz * nz);
    }

    fluidMin = min(p1[1], p2[1]) - zRadius;
    fluidMax = max(p1[1], p2[1]) + zRadius;
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
    double Re = fluid.effectiveDensity*v*D / fluid.effectiveViscosity;

    double k = clamp((Re - 2300) / (4000 - 2300), 0.0, 1.0); // normalize, <0 -> laminar, >1 -> turbulent
    //if (Q > 1e-3) cout << " -- computeRegime " << Q << ", " << v << ", " << Re << " -> " << k << endl;
    return k;
}

void VRPipeSegment::updateResistance(double friction) {
    double fill = max(level, 0.05);
    double rEff = radius * sqrt(fill);
    resistanceLaminar = (8.0 * fluid.effectiveViscosity * length) / (Pi * pow(rEff, 4)); // Hagen–Poiseuille resistance
    resistanceTurbulent = friction * length * fluid.effectiveDensity / ( 4 * rEff * pow(area,2));
    //cout << " resistance: " << resistance << ", L: " << length << ", f: " << friction << ", D: " << density << ", R: " << radius << ", A: " << area << endl;
    if (resistanceLaminar < 1e-9) resistanceLaminar = 1.0;
    if (resistanceTurbulent < 1e-9) resistanceTurbulent = 1.0;
    resistanceLaminar = min(resistanceLaminar, 1e12); // TODO: rework this clamp
    resistanceTurbulent = min(resistanceTurbulent, 1e12); // TODO: rework this clamp
    //resistance = resistanceTurbulent; //max(resistanceLaminar, resistanceTurbulent); // TODO
}

double VRPipeSegment::computeEffectiveResistance(const double& flow) {
    double k = regime;
    double rho_g = fluid.effectiveDensity * gravity;
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

VRPipeSystem::VRPipeSystem() : VRTransform("pipeSystem") {
    addMaterial();
    addEnvironment();

    graph = Graph::create();
    initOntology();

    updateCb = VRUpdateCb::create("pipesSimUpdate", bind(&VRPipeSystem::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRPipeSystem::~VRPipeSystem() {}

VRPipeSystemPtr VRPipeSystem::create() { return VRPipeSystemPtr( new VRPipeSystem() ); }
VRPipeSystemPtr VRPipeSystem::ptr() { return static_pointer_cast<VRPipeSystem>(shared_from_this()); }

VRMaterialPtr VRPipeSystem::setupMaterial() {
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

    return m;
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

    if (type == "Tank") {
        auto fe = ontology->addEntity("fluid", "FluidComposition");
        e->set("fluid", fe->getName());
    }

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

    if (entity->is_a("Cylinder")) {
        double L = entity->getValue("length", 1.0);
        Vec3d o = Vec3d((e->offsetHeight-0.5)*L,0,0);
        e->offset = o;
    }
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

    auto mat = materials[s->materialID];
    s->updateGeometry(graph, mat->friction);
    e1->updateGeometry(graph);
    e2->updateGeometry(graph);
    e1->setInitialHead();
    e2->setInitialHead();
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

Vec2d VRPipeSystem::computeTotalMass() {
    double totalFluidMass = 0.0;
    double totalParticlesMass = 0.0;

    for (auto& n : nodes) { // mass in tanks and cylinders
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 1.0);
            double tankHeight = entity->getValue("height", 1.0);
            double tankVolume = tankArea * tankHeight;
            double tankLevel = entity->getValue("level", 1.0); // 0..1
            double fluidVolume = tankVolume * tankLevel;

            VRFluidComposition tFluid;
            tFluid.fromEntity( entity->getEntity("fluid") );
            totalFluidMass += tFluid.computeFluidMass(fluidVolume);
            totalParticlesMass += tFluid.computeParticlesMass(fluidVolume);
        }

        if (entity->is_a("Cylinder")) {
            double lvl1 = entity->getValue("level1", 1.0);
            double lvl2 = entity->getValue("level2", 1.0);
            double a = entity->getValue("area", 1.0);
            double l = entity->getValue("length", 1.0);
            double cState = entity->getValue("state", 0.0);
            double cVolume = a * l;
            double cVol1 = cVolume * cState;
            double cVol2 = cVolume * (1.0-cState);
            double cFluidVol1 = cVol1 * lvl1;
            double cFluidVol2 = cVol2 * lvl2;

            // TODO: there is currently no fluid composition in cylinder chambers yet
            totalFluidMass += cFluidVol1 * waterDensity;
            totalFluidMass += cFluidVol2 * waterDensity;
        }
    }

    for (auto& s : segments) { // mass in pipes
        auto pipe = s.second;
        double pipeLevel = pipe->level; // 0..1
        double pipeVolume = pipe->volume; // m³
        double fluidVolume = pipeVolume * pipeLevel;
        totalFluidMass += pipe->fluid.computeFluidMass(fluidVolume);
        totalParticlesMass += pipe->fluid.computeParticlesMass(fluidVolume);
    }

    return Vec2d( totalFluidMass, totalParticlesMass );
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
    auto FluidComposition = ontology->addConcept("FluidComposition");
    auto ParticleBin = ontology->addConcept("ParticleBin");

    FluidComposition->addProperty("density", "double");
    FluidComposition->addProperty("temperature", "double");
    FluidComposition->addProperty("viscosity", "double");
    FluidComposition->addProperty("particles", ParticleBin);

    ParticleBin->addProperty("type", "string");
    ParticleBin->addProperty("sizeMin", "double");
    ParticleBin->addProperty("sizeMax", "double");
    ParticleBin->addProperty("density", "double");
    ParticleBin->addProperty("volumeFraction", "double");

    Tank->addProperty("fluid", FluidComposition);
    Tank->addProperty("environmentID", "int");
    Tank->addProperty("materialID", "int");
    Tank->addProperty("pressure", "double");
    Tank->addProperty("pressurized", "double");
    Tank->addProperty("initialGasVolume", "double");
    Tank->addProperty("initialGasPressure", "double");
    Tank->addProperty("area", "double");
    Tank->addProperty("height", "double");
    Tank->addProperty("level", "double");
    Tank->addProperty("isOpen", "bool");

    Gauge->addProperty("pressure", "double");
    Gauge->addProperty("maxPressure", "double");
    Gauge->addProperty("tank", Tank);

    Pump->addProperty("maxHead", "double");
    Pump->addProperty("maxFlow", "double");
    Pump->addProperty("curveExponent", "double");
    Pump->addProperty("rampTime", "double");
    Pump->addProperty("control", "double");
    Pump->addProperty("state", "double");
    Pump->addProperty("newState", "double");
    Pump->addProperty("isOpen", "bool");

    Outlet->addProperty("radius", "double");
    Outlet->addProperty("pressure", "double");
    Outlet->addProperty("density", "double");

    Valve->addProperty("state", "double");
    CheckValve->addProperty("crackingPressure", "double");
    ReliefValve->addProperty("thresholdPressure", "double");
    ReliefValve->addProperty("reseatPressure", "double");
    ControlValve->addProperty("paths", ValvePath);

    // (A,B,x0,xs,K) where A and B are ports and x0 the spool/state position, xs the transition size and K the resistance
    ValvePath->addProperty("A", "int");
    ValvePath->addProperty("B", "int");
    ValvePath->addProperty("x0", "double");
    ValvePath->addProperty("xs", "double");
    ValvePath->addProperty("K", "double");

    Cylinder->addProperty("area", "double");
    Cylinder->addProperty("length", "double");
    Cylinder->addProperty("force", "double"); // external force
    Cylinder->addProperty("damping", "double"); // damps velocity
    Cylinder->addProperty("mass", "double"); // piston mass
    Cylinder->addProperty("level1", "double");
    Cylinder->addProperty("level2", "double");
    Cylinder->addProperty("pressurized1", "bool");
    Cylinder->addProperty("pressurized2", "bool");
    Cylinder->addProperty("state", "double");
    Cylinder->addProperty("headFlow", "double");
    Cylinder->addProperty("pistonSpeed", "double");
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
    auto handlePipe = [&](int eID) {
        auto pipe = segments[eID];
        auto mat = materials[pipe->materialID];
        pipe->updateGeometry(graph, mat->friction);
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        e1->updateGeometry(graph);
        e2->updateGeometry(graph);
    };

    graph->setPosition(nID, p);
    for (auto e : graph->getInEdges(nID)  ) handlePipe(e.ID);
    for (auto e : graph->getOutEdges(nID) ) handlePipe(e.ID);
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
double VRPipeSystem::getSegmentPressure(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? (e1->pressure+e2->pressure)*0.5 : 0.0; }
Vec2d VRPipeSystem::getSegmentGradient(int i) {  auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->pressure, e2->pressure) : Vec2d(); }
Vec2d VRPipeSystem::getSegmentHead(int i) {  auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->hydraulicHead, e2->hydraulicHead) : Vec2d(); }
double VRPipeSystem::getSegmentDensity(int i) { return segments[i]->fluid.effectiveDensity; }
Vec2d VRPipeSystem::getSegmentFlow(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->flow, e2->flow) : Vec2d(); }
Vec2d VRPipeSystem::getSegmentHeadFlow(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->headFlow, e2->headFlow) : Vec2d(); }
Vec2d VRPipeSystem::getSegmentTemperature(int i) { auto e1 = segments[i]->end1.lock(); auto e2 = segments[i]->end2.lock(); return e1 && e2 ? Vec2d(e1->fluid.temperature, e2->fluid.temperature) : Vec2d(); }
double VRPipeSystem::getTankLevel(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("level", 1.0) : 0.0; }
double VRPipeSystem::getPump(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("control", 0.0) : 0.0; }
double VRPipeSystem::getValveState(int nID) { auto e = getNodeEntity(nID); return e ? e->getValue("state", 1.0) : 1.0; }
void VRPipeSystem::setNodeCb(int i, VRAnimCbPtr cb) { if (!nodes.count(i)) return; nodes[i]->userCb = cb; }

void VRPipeSystem::setValve(int nID, double b)  { auto e = getNodeEntity(nID); if (e) e->set("state", toString(b)); }
void VRPipeSystem::setOutletDensity(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("density", toString(p)); }
void VRPipeSystem::setOutletPressure(int nID, double p) { auto e = getNodeEntity(nID); if (e) e->set("pressure", toString(p)); }

void VRPipeSystem::setTankPressure(int nID, double p) {
    auto e = getNodeEntity(nID);
    if (e) e->getEntity("fluid")->set("pressure", toString(p));
}

void VRPipeSystem::setTankTemperature(int nID, double p) {
    auto e = getNodeEntity(nID);
    if (e) e->getEntity("fluid")->set("temperature", toString(p));
}

void VRPipeSystem::setTankDensity(int nID, double p) {
    auto e = getNodeEntity(nID);
    if (e) e->getEntity("fluid")->set("density", toString(p));
}

double VRPipeSystem::getTankPressure(int nID) {
    auto e = getNodeEntity(nID);
    return e ? e->getEntity("fluid")->getValue("pressure", atmosphericPressure) : 0.0;
}

double VRPipeSystem::getTankDensity(int nID) {
    auto e = getNodeEntity(nID);
    return e ? e->getEntity("fluid")->getValue("density", waterDensity) : 0.0;
}

void VRPipeSystem::setPipeRadius(int i, double r) {
    auto mat = materials[segments[i]->materialID];
    segments[i]->radius = r;
    segments[i]->updateGeometry(graph, mat->friction);
    auto e1 = segments[i]->end1.lock();
    auto e2 = segments[i]->end2.lock();
    if (e1) e1->updateGeometry(graph);
    if (e2) e2->updateGeometry(graph);
}

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

void VRPipeSystem::setSegmentMaterial(int sID, int mID) { segments[sID]->materialID = mID; }
int VRPipeSystem::addMaterial() { materials.push_back( MaterialPtr(new Material()) ); return materials.size()-1; }
void VRPipeSystem::setMaterialFriction(int eID, double f) { materials[eID]->friction = f; }
void VRPipeSystem::setMaterialThermalConductivity(int eID, double c) { materials[eID]->thermalConductance = c; }

void VRPipeSystem::setSegmentEnvironment(int sID, int mID) { segments[sID]->environmentID = mID; }
int VRPipeSystem::addEnvironment() { environments.push_back( EnvironmentPtr(new Environment()) ); return environments.size()-1; }
void VRPipeSystem::setEnvironmentVolume(int eID, double v) { environments[eID]->volume = v; }
void VRPipeSystem::setEnvironmentTemperature(int eID, double t) { environments[eID]->temperature = t; }
double VRPipeSystem::getEnvironmentTemperature(int eID) { return environments[eID]->temperature; }

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

    if (!doVisual && !visuals) return;

    if (!doVisual) {
        visuals->destroy();
        visuals = 0;
        return;
    }

    if (!visuals) {
        visuals = VRObject::create("visuals");
        addChild(visuals);

        auto m = setupMaterial();

        auto addVisual = [&](string name) {
            auto v = VRGeometry::create(name);
            v->setMaterial(m);
            visuals->addChild(v);
            return v;
        };

        addVisual("nodeVisuals");
        addVisual("segmentVisuals");
        addVisual("volumeVisuals");
        addVisual("arrowVisuals");
        addVisual("particlesVisuals");
    }

    auto cv = visuals->getChildren();
    VRGeoData dataNodes( dynamic_pointer_cast<VRGeometry>(cv[0]) );
    VRGeoData dataSegments( dynamic_pointer_cast<VRGeometry>(cv[1]) );
    VRGeoData dataVolumes( dynamic_pointer_cast<VRGeometry>(cv[2]) );
    VRGeoData dataArrows( dynamic_pointer_cast<VRGeometry>(cv[3]) );
    VRGeoData dataParticles( dynamic_pointer_cast<VRGeometry>(cv[4]) );

    const Color3f white(1,1,1);
    const Color3f lgray(0.9,0.9,0.9);
    const Color3f yellow(1,1,0);
    const Color3f red(1,0,0);
    const Color3f blue(0.2,0.5,1);
    const Color3f dblue(0.1,0.4,0.7);
    const Color3f lblue(0.3,0.7,1);
    const Color3f green(0.2,1.0,0.2);
    const Color3f brown(0.4,0.2,0.1);


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

        dataNodes.reset();
        dataSegments.reset();
        dataVolumes.reset();
        dataArrows.reset();
        dataParticles.reset();
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
                else if (l == "c") { col1 = green; col2 = green; }
                else if (l == "h") { col1 = green; col2 = green; }
                else continue;

                dataSegments.pushVert(p1+dO*k, norm, col1, tcID1);
                if (l == "h") { dataSegments.pushVert(pm+dO*k, norm, col1, tcID1); dataSegments.pushLine(); } // mid segment
                dataSegments.pushVert(p2+dO*k, norm, col2, tcID2);
                dataSegments.pushLine();
                k++;
            }

            // pipe ends
            dataNodes.pushVert(p1+dPipe*l*0.02, norm, e1->pressurized ? dblue : white, Vec2d());
            dataNodes.pushPoint();
            dataNodes.pushVert(p2-dPipe*l*0.02, norm, e2->pressurized ? dblue : white, Vec2d());
            dataNodes.pushPoint();

            // water box
            if (p2[1] < p1[1]) swap(p1,p2); // p2 always higher than p1
            pm = (p1+p2)*0.5;
            Vec3d d = (p2-p1); d.normalize();

            double g = r*2;
            Vec3d u = Vec3d(1,0,0);
            if (d[1] < 1.0-1e-6) { u = d.cross(Vec3d(0,1,0)); u.normalize(); }
            double a = d.enclosedAngle(Vec3d(0,1,0));
            double t = g*cos(a) + l*sin(a);
            int i0 = dataVolumes.size();
            int k0 = dataVolumes.getNIndices();

            dataVolumes.pushQuad(p1, d, u, Vec2d(g,g), false);
            dataVolumes.pushQuad(pm, Vec3d(0,1,0), u, Vec2d(t,g), true);
            dataVolumes.pushQuad(pm, Vec3d(0,1,0), u, Vec2d(t,g), false);
            dataVolumes.pushQuad(p2, d, u, Vec2d(g,g), false);
            for (int i=0; i<4; i++) dataVolumes.pushColor(dblue);
            for (int i=0; i<4; i++) dataVolumes.pushColor(blue);
            for (int i=0; i<4; i++) dataVolumes.pushColor(white);
            for (int i=0; i<4; i++) dataVolumes.pushColor(white);

            dataVolumes.pushQuad(-9, -10, -11, -12); // mid but reversed
            for (int i=0; i<12; i++) dataVolumes.pushQuad(-1,-1,-1,-1); // placeholders
            updatePipeInds(dataVolumes, 0.5, i0, k0);

            // flow arrows
            i0 = dataArrows.size();
            k0 = dataArrows.getNIndices();

            Vec3d u2 = d.cross(u) + u; u2.normalize();
            float g2 = g*1.05;
            float g3 = g*0.5;
            float g4 = g2/sqrt(2);
            for (int t = 0; t<2; t++) { // 2x2 triangles
                Vec3d pt = (p1+pm)*0.5;
                if (t == 1) pt = (pm+p2)*0.5;

                for (int t = 0; t<2; t++) { // 2 triangles
                    auto col = green;
                    if (t == 1) { g2 *= 0.98; g3 *= 0.98; g4 *= 0.98; col = red; }

                    dataArrows.pushQuad(pt, d, u,  Vec2d(g3,g2), false);
                    dataArrows.pushQuad(pt, d, u,  Vec2d(g2,g3), false);
                    dataArrows.pushQuad(pt, d, u2, Vec2d(g4,g4), false);

                    for (int i=0; i<12; i++) dataArrows.pushColor(col);
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
                        dataArrows.pushQuad(i2+i,i2+i,i0+a,i0+b);
                        dataArrows.pushQuad(i2+i,i2+i,i0+b,i0+a);
                        dataArrows.setNorm(i1+i, dPipe); // store dPipe
                        dataArrows.setNorm(i2+i, Vec3d(dataArrows.getPosition(i2+i))); // store p0
                    }
                }
            }
        }

        auto createWaterBox = [&](Vec3d p, double a, double b, double h) {
            dataVolumes.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), false);
            dataVolumes.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), true);
            dataVolumes.pushQuad(p - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), false);
            dataVolumes.pushQuad(p + Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(a,b), true);
            dataVolumes.pushQuad(-13, -14, -15, -16); // bottom
            dataVolumes.pushQuad(-9, -10, -11, -12); // mid but reversed

            dataVolumes.pushQuad(-9, -10, -14,  -13); // sides bottom
            dataVolumes.pushQuad(-10, -11, -15, -14);
            dataVolumes.pushQuad(-11, -12, -16, -15);
            dataVolumes.pushQuad(-12, -9,  -13, -16);

            dataVolumes.pushQuad(-1, -2, -6,  -5); // sides top
            dataVolumes.pushQuad(-2, -3, -7, -6);
            dataVolumes.pushQuad(-3, -4, -8, -7);
            dataVolumes.pushQuad(-4, -1,  -5, -8);
            for (int i=0; i<4; i++) dataVolumes.pushColor(dblue);
            for (int i=0; i<4; i++) dataVolumes.pushColor(blue);
            for (int i=0; i<4; i++) dataVolumes.pushColor(white);
            for (int i=0; i<4; i++) dataVolumes.pushColor(white);
        };

        auto addParticleBoxVisual = [&](Vec3d p, double a, double h) {
            dataParticles.pushQuad(p, Vec3d(1,0,0), Vec3d(0,1,0), Vec2d(a,h), true);
            for (int i=0; i<4; i++) dataParticles.pushColor(brown);
            dataParticles.pushQuad(-1, -2, -3, -4); // double side
        };

        for (auto& n : nodes) {
            auto p = graph->getPosition(n.first);
            auto e = n.second->entity;

            if (e->is_a("Tank")) {
                double a = e->getValue("area", 1.0);
                double h = e->getValue("height", 1.0);
                double s = sqrt(a);
                createWaterBox(p->pos(), s, s, h);

                VRFluidComposition tFluid;
                tFluid.fromEntity( e->getEntity("fluid") );
                int N = tFluid.particles.size();

                double k = s/(N+1);
                for (int i = 1; i <= N; i++) {
                    addParticleBoxVisual(p->pos()+ Vec3d(i*k-s*0.5,0,0), s*0.5, h);
                }
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

            dataNodes.pushVert(p->pos(), norm, white, Vec2d());
            dataNodes.pushPoint();
        }

        //cout << "apply data: " << data.size() << endl;
        if (dataNodes.size() > 0) dataNodes.apply( dataNodes.getGeometry() );
        if (dataSegments.size() > 0) dataSegments.apply( dataSegments.getGeometry() );
        if (dataVolumes.size() > 0) dataVolumes.apply( dataVolumes.getGeometry() );
        if (dataArrows.size() > 0) dataArrows.apply( dataArrows.getGeometry() );
        if (dataParticles.size() > 0) dataParticles.apply( dataParticles.getGeometry() );
    }

    // update system state

    int iNodes = 0;
    int iSegments = 0;
    int iVolumes = 0;
    int kVolumes = 0;
    int iArrows = 0;
    int iParticles = 0;

    auto flowToArrowLength = [sign](double f, double r, double A, double v0, double v_ref) {
        double v = f/A;
        double F = -sign(v) * 2.0*r*std::asinh(abs(v) / v0) / std::asinh(v_ref / v0);
        return F;
    };

    auto ann = dynamic_pointer_cast<VRAnnotationEngine>( findFirst("testInds") );
    for (auto& s : segments) {
        auto e1 = s.second->end1.lock();
        auto e2 = s.second->end2.lock();
        //float pdelta = s.second->lastPressureDelta; // last written delta, not the correct one
        float pressure1 = e1->pressure;
        float pressure2 = e2->pressure;
        float density = s.second->fluid.effectiveDensity;
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

        float cl1 = e1->flowClamp;
        float cl2 = e2->flowClamp;
        Color3f cc1 = Color3f(cl1, 1.0-cl1, 0);
        Color3f cc2 = Color3f(cl2, 1.0-cl2, 0);

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
            else if (l == "c") { col1 = cc1; col2 = cc2; }
            else if (l == "h") {
                col1 = green; col2 = green;
                auto p1 = dataSegments.getPosition(iSegments)  ; p1[1] = h1; dataSegments.setPos(iSegments  , p1);
                auto p2 = dataSegments.getPosition(iSegments+1); p2[1] = h2; dataSegments.setPos(iSegments+1, p2);
                auto p3 = dataSegments.getPosition(iSegments+2); p3[1] = h3; dataSegments.setPos(iSegments+2, p3);
            } else continue;

            dataSegments.setColor(iSegments, col1); iSegments++;
            dataSegments.setColor(iSegments, col2); iSegments++;
            if (l == "h") iSegments++;
        }

        dataNodes.setColor(iNodes, e1->pressurized ? dblue : lgray); iNodes++;
        dataNodes.setColor(iNodes, e2->pressurized ? dblue : lgray); iNodes++;

        double l = s.second->level;
        //l = 0.5+0.5*sin(F); // TOTEST

        l = clamp(l, 1e-3, 1.0-1e-3);
        if (abs(l-s.second->lastVizLevel) > 1e-3) {
            //cout << "updateVisual - update segment level" << endl;
            s.second->lastVizLevel = l;

            std::array<double, 4> heights;
            heights[0] = dataVolumes.getPosition(iVolumes+ 0)[1];
            heights[1] = dataVolumes.getPosition(iVolumes+ 2)[1];
            heights[2] = dataVolumes.getPosition(iVolumes+12)[1];
            heights[3] = dataVolumes.getPosition(iVolumes+14)[1];
            std::sort(heights.begin(), heights.end());
            double h = heights[0] + l*(heights[3] - heights[0]);

            vector<Pnt3d> intersections;
            auto intersectHz = [&](int a, int b) { // intersect edge (a,b) with horizontal plane at height h
                Pnt3d p1 = dataVolumes.getPosition(iVolumes+a);
                Pnt3d p2 = dataVolumes.getPosition(iVolumes+b);
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
                    dataVolumes.setPos(iVolumes+4+j, intersections[j]);
                    dataVolumes.setPos(iVolumes+8+j, intersections[j]);
                }
            }
            updatePipeInds(dataVolumes, l, iVolumes, kVolumes);

            /*for (int j=0; j<4; j++) {
                if (!s.second->pressurized) dataVolumes.setColor(i+4+j, green);
                else dataVolumes.setColor(i+4+j, blue);
            }*/
        }

        auto tmpCol1 = getTempColor(e1->fluid.temperature);
        auto tmpCol2 = getTempColor(e2->fluid.temperature);

        // color pressurization
        //tmpCol1 = s.second->pressurized ? blue : lblue;
        //tmpCol2 = tmpCol1;

        dataVolumes.setColor(iVolumes+4+0, tmpCol2);
        dataVolumes.setColor(iVolumes+4+1, tmpCol2);
        dataVolumes.setColor(iVolumes+4+2, tmpCol1);
        dataVolumes.setColor(iVolumes+4+3, tmpCol1);

        iVolumes += 16;
        kVolumes += 56;

        // --------- flow arrows -------------
        double r = s.second->radius;
        double A = s.second->area;
        double v0 = 0.5; // m/s
        double v_ref = 4.0; // m/s
        double F1 = flowToArrowLength( e1->flow, r, A, v0, v_ref);
        double F2 = flowToArrowLength(-e2->flow, r, A, v0, v_ref);
        double H1 = flowToArrowLength( e1->headFlow, r, A, v0, v_ref);
        double H2 = flowToArrowLength(-e2->headFlow, r, A, v0, v_ref);

        for (int j=0; j<4; j++) {
            int j1 = iArrows+4+j;
            int j2 = j1+4;
            int j3 = j2+12;
            Vec3d sD = dataArrows.getNormal(j1);
            Vec3d p0 = dataArrows.getNormal(j2);
            dataArrows.setPos(j2, p0 + F1*sD);
            dataArrows.setPos(j3, p0 + H1*sD);
        }

        for (int j=0; j<4; j++) {
            int j1 = iArrows+28+j;
            int j2 = j1+4;
            int j3 = j2+12;
            Vec3d sD = dataArrows.getNormal(j1);
            Vec3d p0 = dataArrows.getNormal(j2);
            dataArrows.setPos(j2, p0 + F2*sD);
            dataArrows.setPos(j3, p0 + H2*sD);
        }

        /*for (int j=0; j<16; j++) {
            Vec3d p = Vec3d(dataVolumes.getPosition(i+j));
            p += Vec3d(j, j, j)*0.0005;
            ann->set(j, p, toString(j));
        }*/

        iArrows += 12*4;
    }

    auto updateWaterBox = [&](double lvl, Color3f col) {
        for (int j=0; j<4; j++) {
            Pnt3d p = dataVolumes.getPosition(iVolumes+j);
            p[1] += lvl;
            dataVolumes.setPos(iVolumes+4+j, p);
            dataVolumes.setPos(iVolumes+8+j, p);
            dataVolumes.setColor(iVolumes+4+j, col);
        }

        iVolumes += 16;
        kVolumes += 48;
    };

    auto updateParticleBox = [&](double h) {
        auto p1 = dataParticles.getPosition(iParticles+0);
        auto p2 = dataParticles.getPosition(iParticles+1);
        dataParticles.setPos(iParticles+2, p1 + Vec3d(0,h,0));
        dataParticles.setPos(iParticles+3, p2 + Vec3d(0,h,0));
        iParticles += 4;
    };

    for (auto& n : nodes) {
        auto e = n.second->entity;
        if (e->is_a("Tank")) {
            VRFluidComposition tFluid;
            tFluid.fromEntity( e->getEntity("fluid") );
            auto col = getTempColor(tFluid.temperature);

            double l = e->getValue("level", 1.0);
            double h = e->getValue("height", 1.0);
            updateWaterBox(h*l, col);

            for (auto& p : tFluid.particles) {
                double sMin = p.second.sizeMin;
                double sMax = p.second.sizeMax;
                double d = p.second.density;
                double vol = p.second.volumeFraction;
                updateParticleBox(vol*l);
            }
            continue;
        }

        if (e->is_a("Cylinder")) {
            double state = e->getValue("state", 1.0);
            double L = e->getValue("length", 1.0);
            Pnt3d p1 = dataVolumes.getPosition(iVolumes+1)    + Vec3d(L*state,0,0);
            Pnt3d p2 = dataVolumes.getPosition(iVolumes+2)    + Vec3d(L*state,0,0);
            Pnt3d p3 = dataVolumes.getPosition(iVolumes+12+1) + Vec3d(L*state,0,0);
            Pnt3d p4 = dataVolumes.getPosition(iVolumes+12+2) + Vec3d(L*state,0,0);
            dataVolumes.setPos(iVolumes+0,    p1);
            dataVolumes.setPos(iVolumes+3,    p2);
            dataVolumes.setPos(iVolumes+12+0, p3);
            dataVolumes.setPos(iVolumes+12+3, p4);
            dataVolumes.setPos(iVolumes+16+1, p1);
            dataVolumes.setPos(iVolumes+16+2, p2);
            dataVolumes.setPos(iVolumes+28+1, p3);
            dataVolumes.setPos(iVolumes+28+2, p4);

            double l1 = e->getValue("level1", 1.0);
            double l2 = e->getValue("level2", 1.0);
            bool chamber1Pressurized = e->getValue("pressurized1", true);
            bool chamber2Pressurized = e->getValue("pressurized2", true);
            double a = e->getValue("area", 1.0);
            double h = sqrt(a);

            Color3f col1 = blue;
            Color3f col2 = blue;
            if (n.second->pipes.size() == 2) {
                col1 = chamber1Pressurized ? blue : lblue;
                col2 = chamber2Pressurized ? blue : lblue;
            }
            updateWaterBox(l1*h, col1);
            updateWaterBox(l2*h, col2);
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

        dataNodes.setColor(iNodes, c); iNodes++;
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

double VRPipeSystem::computeCylinderAccelleration(VRPipeNodePtr node, double dt) {
    return 0; // deprecated

    if (node->pipes.size() != 2) return 0;
    auto entity = node->entity;
    if (!entity->is_a("Cylinder")) return 0;

    auto& e1 = node->pipes[0];
    auto& e2 = node->pipes[1];
    double dH = e2->hydraulicHead - e1->hydraulicHead;

    // compute piston movement and flow
    double Fext = entity->getValue("force", 0.0);
    double d = entity->getValue("damping", 500.0);
    double m = entity->getValue("mass", 500.0);
    double x = entity->getValue("state", 0.0);
    double L = entity->getValue("length", 0.0);
    double A = entity->getValue("area", 0.0);
    double v = entity->getValue("pistonSpeed", 0.0);

    auto p1 = e1->pipe.lock();
    auto p2 = e2->pipe.lock();
    double rho = p1->fluid.effectiveDensity;
    double dP = dH * rho * gravity;
    double Fhyd = -dP*A - v*d;
    double a = (Fhyd - Fext) / m;
    double dv = a*dt;
    if (v > 1e-3 && dv < -2e-3 || v < -1e-3 && dv > 2e-3) v = 0.0; // dont jump 0 in one step
    else v += dv;

    if (abs(v) < 1e-3 || true) cout << "v: " << v << " a: " << a << " dP: " << dP << " Fhyd: " << Fhyd << endl;

    double dx = v * dt / L;
    double x_new = clamp(x + dx, 0.001, 0.999);
    dx = x_new - x;
    v = dx*L/dt;

    entity->set("pistonSpeed", toString(v,12));
    //cout << "cyl speed " << v << endl;

    //static int i=0; i++;
    //if (i<50) cout << "cyl acc, v " << v << ", x " << x << ", dx " << dx << ", dH " << dH << ", h1 " << e1->hydraulicHead << ", h2 " << e2->hydraulicHead << endl;
    //if (abs(v) > 1.0) cout << "cyl acc, v " << v << ", dx " << dx << ", dH " << dH << ", h1 " << e1->hydraulicHead << ", h2 " << e2->hydraulicHead << endl;
    //if (entity->getName() == "cylinder") cout << "cyl acc, v " << v << ", dx " << dx << ", dH " << dH << endl;


    double hflow = A*v;
    return hflow;
}

void VRPipeSystem::updateNodePaths() {
    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (entity->is_a("Junction")) continue;

        auto& ends = node->pipes;
        auto& endGroups = node->endGroups;
        auto& endsGroup = node->endsGroup;
        auto& pathOpenings = node->pathOpenings;
        endGroups.clear();
        endsGroup.clear();
        pathOpenings.clear();
        int Ne = (int)ends.size();

        if (entity->is_a("ControlValve")) {
            auto paths = entity->getAllEntities("paths");
            double x = entity->getValue("state", 0.0);

            for (auto& p : paths) {
                int A = p->getValue("A", -1);
                int B = p->getValue("B", -1);
                double x0 = p->getValue("x0", 0.0);
                double xs = p->getValue("xs", 0.0);
                double K  = p->getValue("K" , 0.0);

                if (A<0 || B<0 || A>=Ne || B>=Ne) {
                    cout << "Error in processControlValve, wrong ends indices: " << A << ", " << B << endl;
                    continue;
                }

                double y = K * max(0.0, 1.0-abs(x-x0)/xs); // tent function around x0

                if (y > 1e-3) { // connected
                    if (endsGroup.count(A)) endsGroup[B] = endsGroup[A];
                    else if (endsGroup.count(B)) endsGroup[A] = endsGroup[B];
                    else {
                        int g = endsGroup.size();
                        endsGroup[A] = g;
                        endsGroup[B] = g;
                    }

                    if (!pathOpenings.count(A)) pathOpenings[A] = 0;
                    if (!pathOpenings.count(B)) pathOpenings[B] = 0;
                    pathOpenings[A] += y;
                    pathOpenings[B] += y;
                }
            }
        } else if (entity->is_a("Valve")) {
            if (Ne < 2) continue;
            double x = entity->getValue("state", 0.0);

            if (x > 1e-3) {
                endsGroup[0] = 0;
                endsGroup[1] = 0;
                pathOpenings[0] = x;
                pathOpenings[1] = x;
            }
        } else if (entity->is_a("Pump")) {
            if (Ne < 2) continue;
            double x = entity->getValue("state", 0.0);
            bool isOpen = entity->getValue("isOpen", false);

            if (x > 1e-3) {
                endsGroup[0] = 0;
                endsGroup[1] = 0;
                pathOpenings[0] = 1.0;
                pathOpenings[1] = 1.0;
            } else {
                if (isOpen) {
                    endsGroup[0] = 0;
                    endsGroup[1] = 0;
                    pathOpenings[0] = 0.5;
                    pathOpenings[1] = 0.5;
                }
            }
        }

        for (auto& eg : endsGroup) {
            if (!endGroups.count(eg.second)) endGroups[eg.second] = vector<VRPipeEndPtr>();
            endGroups[eg.second].push_back(ends[eg.first]);
        }
    }

}

void VRPipeSystem::assignBoundaryPressures(double dt) {
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
                e->hydraulicHead = e->height + outletPressure/pipe->fluid.effectiveDensity/gravity;
            }
            continue;
        }

        if (entity->is_a("Tank")) {
            VRFluidComposition tFluid;
            tFluid.fromEntity( entity->getEntity("fluid") );

            double tankHeight = entity->getValue("height", 1.0);
            double tankLevel = entity->getValue("level", 1.0);
            double tankDensity = tFluid.effectiveDensity;
            bool tankOpen = entity->getValue("isOpen", false);
            bool tankPressurized = entity->getValue("pressurized", false);
            if (tankPressurized) continue; // just a full tank

            double tankPressure = entity->getValue("pressure", atmosphericPressure);
            if (tankOpen) tankPressure = atmosphericPressure;

            double fluidEffect = 1.0; // used to remove numerical artifacts of empty tanks
            double eps = 1e-3;
            if (tankLevel < eps) fluidEffect = tankLevel/eps;

            double fluidHeight = (tankLevel-0.5)*tankHeight + nPos[1];
            for (auto& e : node->pipes) {
                double depth = max(0.0, fluidHeight - e->height);
                double Pfluid = depth * tankDensity * gravity;
                double Pgauge = tankPressure + Pfluid - atmosphericPressure;
                double hydrHead = e->height + fluidEffect * Pgauge / (tankDensity * gravity);
                e->hydraulicHead = hydrHead;
                /*if (n.first == 4)
                cout << " tank boundary expr.: hH: " << e->hydraulicHead
                    << ", tankOpen: " << tankOpen << ", d: " << depth
                    << ", tP " << tankPressure << ", fP " << Pfluid << ", gP " << Pgauge
                    << " rho: " << tankDensity
                    << endl;*/
            }
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;

            computeCylinderAccelleration(node, dt);

            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];

            double l1 = entity->getValue("level1", 1.0);
            double l2 = entity->getValue("level2", 1.0);
            bool chamber1Pressurized = entity->getValue("pressurized1", true);
            bool chamber2Pressurized = entity->getValue("pressurized2", true);
            double a = entity->getValue("area", 1.0);
            double h = sqrt(a);
            double h1 = nPos[1] + h*(l1-0.5);
            double h2 = nPos[1] + h*(l2-0.5);
            if (!chamber1Pressurized) e1->hydraulicHead = h1;
            if (!chamber2Pressurized) e2->hydraulicHead = h2;
            //cout << "bcond cyl " << h1 << "/" << h2 << "  " << e1->hydraulicHead << "/" << e2->hydraulicHead << endl;
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        if (pipe->pressurized && e1->pressurized && e2->pressurized) continue;

        if (!pipe->pressurized) {
            //cout << " set unpressurized pipe head to " << pipe->fluidLvl << endl;
            pipe->hydraulicHead = pipe->fluidLvl;
            continue;
        }
        //if (pipe->eID == 48) cout << " pipe48 fluidLvl: " << pipe->fluidLvl << ", head: " << pipe->hydraulicHead << endl;

        if (pipe->fluidLvl > pipe->hydraulicHead) {
            //cout << " set pressurized pipe head to " << pipe->fluidLvl << endl;
            pipe->hydraulicHead = pipe->fluidLvl;
        }
    }
}

class SparseMatrix {
    public:
        struct Row {
            vector<pair<int, double>> cells;

            double& operator[](int i) {
                for (auto& c : cells) if(i == c.first) return c.second;
                cells.push_back(make_pair(i,0.0));
                return cells.back().second;
            }

            double get(int i) const {
                for (auto& c : cells) if (c.first == i) return c.second;
                return 0.0;
            };
        };

    public:
        vector<Row> rows;

    public:
        SparseMatrix(int Nrows) : rows(Nrows) {}

        Row& operator[](int i) {
            return rows[i];
        }
};

void VRPipeSystem::solveNodeHeads(double dt) {
    struct Solver {
        int N = 0;
        SparseMatrix   A;
        vector<double> x;
        vector<double> b;

        Solver(int n)
            : N(n)
            , A(n)
            , x(n)
            , b(n, 0.0)
        {}
    };

    auto solveLinearSystem2 = [](Solver& solver) -> bool {
        auto& A = solver.A;
        auto& b = solver.b;
        auto& x = solver.x;

        const int N = solver.N;
        const double tol = 1e-8;
        const double eps = 1e-30;
        const int maxIter = max(50, N);

        if ((int)x.size() != N) x.assign(N, 0.0);

        vector<double> Ax(N), r(N), r0(N);
        vector<double> p(N), phat(N);
        vector<double> v(N);
        vector<double> s(N), shat(N);
        vector<double> t(N);
        vector<double> Minv(N, 1.0);

        double rhoOld = 1.0;
        double rhoNew = 1.0;
        double alpha  = 1.0;
        double omega  = 1.0;
        double beta   = 0.0;
        double normB  = 1.0;
        double res    = 1e30;

        int restartEvery = 50;

        auto dot = [&](const vector<double>& a, const vector<double>& b) {
            double sum = 0.0;
            for (int i = 0; i < N; ++i) sum += a[i] * b[i];
            return sum;
        };

        auto norm = [&](const vector<double>& a) {
            return sqrt(dot(a, a));
        };

        auto multiply = [&](const vector<double>& in, vector<double>& out) {
            out.assign(N, 0.0);

            for (int i = 0; i < N; ++i) {
                double sum = 0.0;

                for (auto& c : A[i].cells) {
                    int j = c.first;
                    double aij = c.second;
                    sum += aij * in[j];
                }

                out[i] = sum;
            }
        };

        auto applyPreconditioner = [&](const vector<double>& in, vector<double>& out) {
            out.resize(N);

            for (int i = 0; i < N; ++i) {
                out[i] = Minv[i] * in[i];
            }
        };

        auto scaleRows = [&]() {
            for (int i = 0; i < solver.N; ++i) {
                double m = 0.0;

                for (auto& c : solver.A[i].cells) {
                    m = max(m, abs(c.second));
                }

                if (m < 1e-30) continue;

                double s = 1.0 / m;

                for (auto& c : solver.A[i].cells) {
                    c.second *= s;
                }

                solver.b[i] *= s;
            }
        };

        auto buildPreconditioner = [&]() {
            for (int i = 0; i < N; ++i) {
                double d = A[i].get(i);

                if (abs(d) > 1e-20) {
                    Minv[i] = 1.0 / d;
                } else {
                    Minv[i] = 1.0;
                }
            }
        };

        auto computeInitialResidual = [&]() {
            multiply(x, Ax);

            for (int i = 0; i < N; ++i) {
                r[i] = b[i] - Ax[i];
            }

            r0 = r;

            normB = max(norm(b), 1.0);
            res = norm(r) / normB;

            p.assign(N, 0.0);
            v.assign(N, 0.0);

            rhoOld = 1.0;
            alpha  = 1.0;
            omega  = 1.0;
        };

        auto updateSearchDirection = [&](int iter) -> bool {
            rhoNew = dot(r0, r);

            if (abs(rhoNew) < eps) {
                cout << "BiCGSTAB failed: rho breakdown at iter " << iter << endl;
                return false;
            }

            if (abs(omega) < eps) {
                cout << "BiCGSTAB failed: omega breakdown before beta at iter " << iter << endl;
                return false;
            }

            beta = (rhoNew / rhoOld) * (alpha / omega);

            for (int i = 0; i < N; ++i) {
                p[i] = r[i] + beta * (p[i] - omega * v[i]);
            }

            return true;
        };

        auto solveSearchDirection = [&]() -> bool {
            applyPreconditioner(p, phat);
            multiply(phat, v);

            double r0v = dot(r0, v);

            if (abs(r0v) < eps) {
                cout << "BiCGSTAB failed: alpha breakdown" << endl;
                return false;
            }

            alpha = rhoNew / r0v;
            return true;
        };

        auto computeIntermediateResidual = [&]() {
            for (int i = 0; i < N; ++i) {
                s[i] = r[i] - alpha * v[i];
            }
        };

        auto acceptIntermediateSolution = [&]() -> bool {
            double sRes = norm(s) / normB;

            if (sRes >= tol)
                return false;

            for (int i = 0; i < N; ++i) {
                x[i] += alpha * phat[i];
            }

            return true;
        };

        auto solveIntermediateDirection = [&]() -> bool {
            applyPreconditioner(s, shat);
            multiply(shat, t);

            double tt = dot(t, t);

            if (abs(tt) < eps) {
                cout << "BiCGSTAB failed: omega denominator breakdown" << endl;
                return false;
            }

            omega = dot(t, s) / tt;

            if (abs(omega) < eps) {
                cout << "BiCGSTAB failed: omega zero" << endl;
                return false;
            }

            return true;
        };

        auto updateSolution = [&]() {
            for (int i = 0; i < N; ++i) {
                x[i] += alpha * phat[i] + omega * shat[i];
                r[i]  = s[i] - omega * t[i];
            }

            res = norm(r) / normB;
            rhoOld = rhoNew;
        };

        auto computeMaxResidual = [&]() {
            multiply(x, Ax);

            double maxR = 0.0;
            int maxRow = -1;

            for (int i = 0; i < N; ++i) {
                double ri = abs(b[i] - Ax[i]);
                if (ri > maxR) {
                    maxR = ri;
                    maxRow = i;
                }
            }

            return pair<double,int>(maxR, maxRow);
        };

        auto computeMaxCorrection = [&](const vector<double>& xOld) {
            double maxDx = 0.0;
            int maxI = -1;

            for (int i = 0; i < N; ++i) {
                double dx = abs(x[i] - xOld[i]);
                if (dx > maxDx) {
                    maxDx = dx;
                    maxI = i;
                }
            }

            return pair<double,int>(maxDx, maxI);
        };

        vector<double> xStart = x;
        scaleRows();
        buildPreconditioner();
        computeInitialResidual();
        double resStart = res;
        if (res < tol) return true;

        int iter;
        for (iter = 0; iter < maxIter; ++iter) {
            if (iter > 0 && iter % restartEvery == 0) {
                r0 = r;
                rhoOld = 1.0;
                alpha = 1.0;
                omega = 1.0;
                fill(p.begin(), p.end(), 0.0);
                fill(v.begin(), v.end(), 0.0);
            }

            if (!updateSearchDirection(iter)) return false;
            if (!solveSearchDirection()) return false;
            computeIntermediateResidual();
            if (acceptIntermediateSolution()) return true;
            if (!solveIntermediateDirection()) return false;
            updateSolution();
            if (res < tol) return true;
        }

        auto [maxR, maxRow] = computeMaxResidual();
        auto [maxDx, maxDxID] = computeMaxCorrection(xStart);

        cout << "BiCGSTAB"
             << " iter " << iter
             << " res " << resStart << " -> " << res
             << " maxR " << maxR << " row " << maxRow
             << " maxDx " << maxDx << " id " << maxDxID
             << endl;

        double minDiag = 1e300;
        double maxDiag = 0.0;
        double maxCoeff = 0.0;
        double minCoeff = 1e300;
        int zeroDiag = 0;

        for (int i = 0; i < N; ++i) {
            double d = abs(A[i].get(i));
            if (d < 1e-30) zeroDiag++;
            else {
                minDiag = min(minDiag, d);
                maxDiag = max(maxDiag, d);
            }

            for (auto& c : A[i].cells) {
                double a = abs(c.second);
                if (a < 1e-30) continue;
                minCoeff = min(minCoeff, a);
                maxCoeff = max(maxCoeff, a);
            }
        }

        cout << "A stats"
             << " minDiag " << minDiag
             << " maxDiag " << maxDiag
             << " zeroDiag " << zeroDiag
             << " minCoeff " << minCoeff
             << " maxCoeff " << maxCoeff
             << endl;

        cout << "BiCGSTAB failed: no convergence, residual " << res << endl;
        return false;
    };

    auto solveLinearSystem = [](Solver& solver) -> bool {
        auto& A = solver.A;
        auto& b = solver.b;
        auto& x = solver.x;

        int N = b.size();
        x.assign(N, 0.0);

        const double eps = 1e-12;

        // Forward elimination: convert A to upper triangular form.
        for (int i = 0; i < N; i++) {
            int pivot = i;
            double maxAbs = abs(A[i].get(i));

            for (int r = i + 1; r < N; r++) {
                double v = abs(A[r].get(i));
                if (v > maxAbs) {
                    maxAbs = v;
                    pivot = r;
                }
            }

            if (maxAbs < eps) {
                cout << "solveLinearSystem failed, singular at row " << i << endl;
                return false;
            }

            if (pivot != i) {
                swap(A[i], A[pivot]);
                swap(b[i], b[pivot]);
            }

            double div = A[i].get(i);

            for (auto& c : A[i].cells) {
                c.second /= div;
            }
            b[i] /= div;

            // Only eliminate below the pivot.
            for (int r = i + 1; r < N; r++) {
                double f = A[r].get(i);
                if (abs(f) < eps) continue;

                for (auto& cell : A[i].cells) {
                    int c = cell.first;
                    if (c < i) continue;
                    A[r][c] -= f * cell.second;
                }

                b[r] -= f * b[i];

                // Keep the pivot column explicitly zero.
                A[r][i] = 0.0;
            }
        }

        // Back substitution.
        for (int i = N - 1; i >= 0; i--) {
            double rhs = b[i];

            for (auto& cell : A[i].cells) {
                int c = cell.first;
                if (c <= i) continue;
                rhs -= cell.second * x[c];
            }

            double div = A[i].get(i);
            if (abs(div) < eps) {
                cout << "solveLinearSystem failed during back substitution at row " << i << endl;
                return false;
            }

            x[i] = rhs / div;
        }

        return true;
    };

    auto solveLinearSystem3 = [](Solver& solver) -> bool {
        auto& A = solver.A;
        auto& b = solver.b;
        auto& x = solver.x;

        const int N = solver.N;
        //const double tol = 1e-8;
        const double eps = 1e-30;
        //const int restart = 40;        // GMRES(m)
        //const int maxCycles = max(5, N / restart + 2);

        const int restart = 10;
        const int maxCycles = 10;
        const double tol = 1e-10;

        if ((int)x.size() != N) x.assign(N, 0.0);

        vector<double> Ax(N), r(N), w(N);
        vector<double> Minv(N, 1.0);

        auto dot = [&](const vector<double>& a, const vector<double>& b) {
            double s = 0.0;
            for (int i = 0; i < N; ++i) s += a[i] * b[i];
            return s;
        };

        auto norm = [&](const vector<double>& a) {
            return sqrt(dot(a, a));
        };

        auto multiply = [&](const vector<double>& in, vector<double>& out) {
            out.assign(N, 0.0);

            for (int i = 0; i < N; ++i) {
                double s = 0.0;
                for (auto& c : A[i].cells) {
                    s += c.second * in[c.first];
                }
                out[i] = s;
            }
        };

        auto buildPreconditioner = [&]() {
            for (int i = 0; i < N; ++i) {
                double d = A[i].get(i);
                Minv[i] = abs(d) > 1e-20 ? 1.0 / d : 1.0;
            }
        };

        auto applyPreconditioner = [&](const vector<double>& in, vector<double>& out) {
            out.resize(N);
            for (int i = 0; i < N; ++i) out[i] = Minv[i] * in[i];
        };

        auto computeResidual = [&]() {
            multiply(x, Ax);
            for (int i = 0; i < N; ++i) r[i] = b[i] - Ax[i];
        };

        auto applyApreconditioned = [&](const vector<double>& in, vector<double>& out) {
            // right-preconditioned GMRES:
            // Krylov basis is built for A * M^-1.
            vector<double> z(N);
            applyPreconditioner(in, z);
            multiply(z, out);
        };

        auto solveSmallLeastSquares = [&](int k,
                                          vector<vector<double>>& H,
                                          vector<double>& g,
                                          vector<double>& y) {
            // Solve min || g - H y || using normal equations.
            // Size is small: k <= restart.
            y.assign(k, 0.0);

            vector<vector<double>> B(k, vector<double>(k, 0.0));
            vector<double> rhs(k, 0.0);

            for (int i = 0; i < k; ++i) {
                for (int row = 0; row <= k; ++row) {
                    rhs[i] += H[row][i] * g[row];

                    for (int j = 0; j < k; ++j) {
                        B[i][j] += H[row][i] * H[row][j];
                    }
                }
            }

            // Dense Gaussian solve for tiny k x k system.
            for (int i = 0; i < k; ++i) {
                int pivot = i;
                double maxAbs = abs(B[i][i]);

                for (int r = i + 1; r < k; ++r) {
                    double v = abs(B[r][i]);
                    if (v > maxAbs) {
                        maxAbs = v;
                        pivot = r;
                    }
                }

                if (maxAbs < eps) return false;

                if (pivot != i) {
                    swap(B[i], B[pivot]);
                    swap(rhs[i], rhs[pivot]);
                }

                double div = B[i][i];
                for (int c = i; c < k; ++c) B[i][c] /= div;
                rhs[i] /= div;

                for (int r = i + 1; r < k; ++r) {
                    double f = B[r][i];
                    if (abs(f) < eps) continue;

                    for (int c = i; c < k; ++c) {
                        B[r][c] -= f * B[i][c];
                    }

                    rhs[r] -= f * rhs[i];
                }
            }

            for (int i = k - 1; i >= 0; --i) {
                double v = rhs[i];

                for (int j = i + 1; j < k; ++j) {
                    v -= B[i][j] * y[j];
                }

                if (abs(B[i][i]) < eps) return false;
                y[i] = v / B[i][i];
            }

            return true;
        };

        auto updateSolution = [&](int k,
                                  vector<vector<double>>& V,
                                  vector<double>& y) {
            // x += M^-1 * V*y
            vector<double> dx(N, 0.0);
            vector<double> z(N);

            for (int j = 0; j < k; ++j) {
                applyPreconditioner(V[j], z);

                for (int i = 0; i < N; ++i) {
                    dx[i] += y[j] * z[i];
                }
            }

            for (int i = 0; i < N; ++i) {
                x[i] += dx[i];
            }
        };

        auto computeMaxCorrection = [&](const vector<double>& xOld) {
            double maxDx = 0.0;
            int maxI = -1;

            for (int i = 0; i < N; ++i) {
                double dx = abs(x[i] - xOld[i]);
                if (dx > maxDx) {
                    maxDx = dx;
                    maxI = i;
                }
            }

            return pair<double,int>(maxDx, maxI);
        };

        buildPreconditioner();

        double normB = max(norm(b), 1.0);

        computeResidual();
        double beta = norm(r);
        double res = beta / normB;

        if (res < tol) return true;
        vector<double> xStart = x;

        for (int cycle = 0; cycle < maxCycles; ++cycle) {
            vector<vector<double>> V(restart + 1, vector<double>(N, 0.0));
            vector<vector<double>> H(restart + 1, vector<double>(restart, 0.0));
            vector<double> g(restart + 1, 0.0);
            vector<double> y;

            beta = norm(r);
            if (beta < eps) return true;

            for (int i = 0; i < N; ++i) {
                V[0][i] = r[i] / beta;
            }

            g[0] = beta;

            int usedK = 0;

            for (int k = 0; k < restart; ++k) {
                applyApreconditioned(V[k], w);

                // Arnoldi orthogonalization
                for (int j = 0; j <= k; ++j) {
                    H[j][k] = dot(w, V[j]);

                    for (int i = 0; i < N; ++i) {
                        w[i] -= H[j][k] * V[j][i];
                    }
                }

                // Re-orthogonalization, improves stability.
                for (int j = 0; j <= k; ++j) {
                    double h2 = dot(w, V[j]);
                    H[j][k] += h2;

                    for (int i = 0; i < N; ++i) {
                        w[i] -= h2 * V[j][i];
                    }
                }

                H[k + 1][k] = norm(w);
                usedK = k + 1;

                if (H[k + 1][k] > eps) {
                    for (int i = 0; i < N; ++i) {
                        V[k + 1][i] = w[i] / H[k + 1][k];
                    }
                }

                if (!solveSmallLeastSquares(usedK, H, g, y)) {
                    cout << "GMRES failed: small least-squares solve failed" << endl;
                    return false;
                }

                // Cheap enough for debugging: form tentative x and compute true residual.
                vector<double> xOld = x;
                updateSolution(usedK, V, y);

                computeResidual();
                res = norm(r) / normB;
                if (res < tol) return true;

                x = xOld;
                if (H[k + 1][k] <= eps) break;
            }

            if (!solveSmallLeastSquares(usedK, H, g, y)) {
                cout << "GMRES failed: final least-squares solve failed" << endl;
                return false;
            }

            updateSolution(usedK, V, y);
            computeResidual();
            res = norm(r) / normB;
            if (res < tol) return true;

            auto [maxDx, maxDxID] = computeMaxCorrection(xStart);
            cout << "cycle: " << cycle << " -> maxDx: " << maxDx << endl;
            if (maxDx < 2e-3) return true;
            xStart = x;
        }

        cout << "GMRES failed: no convergence, residual " << res << endl;
        return false;
    };

    auto distributeStateIDs = [&]() {
        int i = 0;
        for (auto& s : segments) { // set state IDs
            auto& pipe = s.second;
            auto e1 = pipe->end1.lock();
            auto e2 = pipe->end2.lock();
            pipe->stateID = i+0;
            e1->stateID   = i+1;
            e2->stateID   = i+2;
            i += 3;
        }

        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;
            if (!entity) continue;

            if (entity->is_a("Cylinder")) {
                node->stateID = i; i++;
            }
        }
        return i;
    };

    auto buildStateVector = [&](vector<double>& x) {
        for (auto& s : segments) { // build state vector
            auto& pipe = s.second;
            auto e1 = pipe->end1.lock();
            auto e2 = pipe->end2.lock();
            x[pipe->stateID] = pipe->hydraulicHead;
            x[e1->stateID]   = e1->hydraulicHead;
            x[e2->stateID]   = e2->hydraulicHead;
        }

        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;
            if (!entity) continue;

            if (entity->is_a("Cylinder")) {
                x[node->stateID] = entity->getValue("pistonSpeed", 0.0);
            }
        }
    };

    auto updateHeads = [&](vector<double>& x) {
        for (auto& s : segments)  { // update heads
            auto& pipe = s.second;
            auto e1 = pipe->end1.lock();
            auto e2 = pipe->end2.lock();
            pipe->hydraulicHead = x[pipe->stateID];
            e1->hydraulicHead   = x[e1->stateID];
            e2->hydraulicHead   = x[e2->stateID];
        }
    };

    auto computePumpHead = [&](VREntityPtr entity, double flow) {
        double mH = entity->getValue("maxHead", 0.0);
        double mQ = entity->getValue("maxFlow", 1.0);
        double e = entity->getValue("curveExponent", 2.0);
        double H = mH * ( 1.0 - pow(flow / mQ,e) );
        //cout << "computePumpHead Q: " << flow << ", H " << H << ", e " << e << endl;
        return H;
    };

    auto setDirichlet = [&](Solver& solver, int i, double H) {
        solver.A[i].cells.clear();
        solver.A[i][i] = 1.0;
        solver.b[i] = H;
    };

    auto addFlowBalance = [&](Solver& solver, vector<VRPipeEndPtr> ends) { // sum(Qi) = 0
        vector<pair<int,double>> links;
        double Gsum = 0.0;

        for (auto e : ends) {
            auto pipe = e->pipe.lock();
            double R = pipe->computeEffectiveResistance(e->flow);
            double G = 2.0 / max(R, 1e-9);
            Gsum += G;
            links.push_back({ pipe->stateID, G });
        }

        for (auto e : ends) {
            int i = e->stateID;
            solver.A[i][i] += Gsum;
            solver.b[i] = 0.0;
            for (auto& l : links) solver.A[i][l.first] -= l.second;
        }
    };

    auto balancePipeFlow = [&](Solver& solver, int centerID, vector<pair<int,double>> links) { // sum(Qi) = 0
        // links: {neighborStateID, conductance}

        double Gsum = 0.0;

        for (auto& l : links) {
            int j = l.first;
            double G = l.second;

            Gsum += G;
            solver.A[centerID][j] -= G;
        }

        solver.A[centerID][centerID] += Gsum;
        solver.b[centerID] = 0.0;
    };

    auto addDeadEnd = [&](Solver& solver, VRPipeEndPtr e) {
        auto pipe = e->pipe.lock();
        int i = e->stateID;
        int pi = pipe->stateID;

        solver.A[i][i]  =  1.0;
        solver.A[i][pi] = -1.0;
    };

    auto addChamberFlow = [&](Solver& solver, VRPipeEndPtr e, int vID, double A) {
        auto pipe = e->pipe.lock();

        int i  = e->stateID;
        int pi = pipe->stateID;

        double R = pipe->computeEffectiveResistance(e->flow);
        double G = 2.0 / max(R, 1e-9); // half-pipe conductance

        /*if (e->flow > 1e-3) {
            cout << "addChamberFlow"
                << " eFlow " << e->flow
                << " R " << R
                << " G " << G
                << endl;
        }*/

        // Desired flow:
        //
        //   Qdes = G * (Hpipe - Hend)
        //
        // Therefore:
        //
        //   Hend - Hpipe = -Qdes / G

        solver.A[i][i]  =  1.0;
        solver.A[i][pi] = -1.0;

        //solver.b[i]     = -Qdes / G;
        solver.A[i][vID] = A / G;
        solver.b[i] = 0.0;

        /*if (i == 2 && Qdes < 0.0 || true)
        cout << "addChamberFlow " << i
            << " pipeRegime " << pipe->regime
            << " R " << R
            << " G " << G
            << " Qdes " << Qdes
            << endl;*/
    };

    auto detectHydraulicIslands = [&](Solver& solver) {
        double eps = 1e-12;
        auto& N = solver.N;

        vector<vector<int>> adj(N);

        for (int i = 0; i < N; i++) {
            for (auto& c : solver.A[i].cells) {
                int j = c.first;
                double v = c.second;

                if (i == j) continue;
                if (abs(v) > eps) {
                    adj[i].push_back(j);
                    adj[j].push_back(i);
                }
            }
        }

        auto isPinnedRow = [&](int i) {
            int nz = 0;
            bool hasDiag = false;

            for (auto& c : solver.A[i].cells) {
                int j = c.first;
                double v = c.second;
                if (abs(v) <= eps) continue;
                nz++;
                if (j == i) hasDiag = true;
            }

            return nz == 1 && hasDiag;
        };

        vector<int> nodeGroup(solver.N, -1);
        int Ngroups = 0;

        for (int i = 0; i < N; i++) {
            if (nodeGroup[i] >= 0) continue;

            vector<int> stack = { i };
            nodeGroup[i] = Ngroups;

            while (!stack.empty()) {
                int k = stack.back();
                stack.pop_back();

                for (int j : adj[k]) {
                    if (nodeGroup[j] >= 0) continue;
                    nodeGroup[j] = Ngroups;
                    stack.push_back(j);
                }
            }

            Ngroups++;
        }

        vector<bool> hasPin(Ngroups, false);
        vector<int> firstGroupNode(Ngroups, -1);

        for (int i = 0; i < N; i++) {
            int g = nodeGroup[i];
            if (firstGroupNode[g] < 0) firstGroupNode[g] = i;
            if (isPinnedRow(i)) hasPin[g] = true;
        }

        for (int g = 0; g < Ngroups; g++) {
            if (hasPin[g]) continue;
            int i = firstGroupNode[g];
            if (i < 0) continue;
            setDirichlet(solver, i, solver.x[i]);
        }
    };

    auto timer = VRTimer::create();

    int N = distributeStateIDs();
    Solver solver(N);
    buildStateVector(solver.x);

    auto T3 = timer->stop();

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();

        int p  = pipe->stateID;
        int i1 = e1->stateID;
        int i2 = e2->stateID;

        double R = pipe->computeEffectiveResistance(e1->flow);
        double G = 2.0 / max(R, 1e-9);

        if (pipe->pressurized) { // Q1 + Q2 = 0 -> G(Hpipe - H1) + G(Hpipe - H2) = 0
            balancePipeFlow(solver, p, { { i1, G }, { i2, G } });
        } else {
            setDirichlet(solver, p, pipe->fluidLvl);
        }
    }

    auto T4 = timer->stop();
    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (!entity) continue;
        auto& ends = node->pipes;

        if (entity->is_a("Outlet")) {
            for (auto e : node->pipes) setDirichlet(solver, e->stateID, e->hydraulicHead);
            continue;
        }

        if (entity->is_a("Tank")) {
            bool io = entity->getValue("isOpen", false);
            bool tankPressurized = entity->getValue("pressurized", false);

            if (io || !tankPressurized) {
                for (auto e : ends) {
                     setDirichlet(solver, e->stateID, e->hydraulicHead);
                }
                continue;
            }

            addFlowBalance(solver, ends);
            continue;
        }

        if (entity->is_a("Pump")) {
            if (ends.size() == 2) {
                auto e1 = ends[0]; // suction
                auto e2 = ends[1]; // discharge

                auto pS = e1->pipe.lock();
                auto pD = e2->pipe.lock();

                int s  = e1->stateID;
                int d  = e2->stateID;
                int ps = pS->stateID;
                int pd = pD->stateID;

                double RS = pS->computeEffectiveResistance(e1->flow);
                double RD = pD->computeEffectiveResistance(e2->flow);

                double GS = 2.0 / max(RS, 1e-9); // half-pipe conductance
                double GD = 2.0 / max(RD, 1e-9);

                double control  = entity->getValue("control", 0.0);
                double state    = entity->getValue("state", 0.0);
                double rampTime = entity->getValue("rampTime", 0.5);
                bool   isOpen   = entity->getValue("isOpen", false);

                double alpha = clamp(dt / max(rampTime, 1e-9), 0.0, 1.0);
                state += ((control > state) - (control < state)) * alpha;
                state = clamp(state, 0.0, 1.0);
                entity->set("newState", toString(state, 12));

                if (state < 1e-3 && !isOpen) {
                    // Pump closed: no flow through either pump port.
                    //
                    // QS = 0 -> HS = HpipeS
                    // QD = 0 -> HD = HpipeD
                    addDeadEnd(solver, e1);
                    addDeadEnd(solver, e2);
                    continue;
                }

                double pumpGain = 0.0;

                if (state >= 1e-3) {
                    // For now, use previous timestep flow for pump curve.
                    // This matches your old solver behavior.
                    double qPump = max(0.0, e1->flow);

                    double pumpHead = computePumpHead(entity, qPump);
                    pumpGain = state * max(0.0, pumpHead);
                }

                // Equation 1: pump continuity
                //
                // GS(HS - HpipeS) + GD(HD - HpipeD) = 0
                solver.A[s][s]  += GS;
                solver.A[s][ps] -= GS;
                solver.A[s][d]  += GD;
                solver.A[s][pd] -= GD;

                // Equation 2: pump head jump
                //
                // HD - HS = pumpGain
                //
                // If pump is off but open, pumpGain = 0, so this becomes HD = HS.
                solver.A[d][d] =  1.0;
                solver.A[d][s] = -1.0;
                solver.b[d] = pumpGain;

                continue;
            }
        }

        if (entity->is_a("Cylinder")) {
            if (ends.size() == 2) {
                auto& e1 = ends[0];
                auto& e2 = ends[1];
                bool chamber1Pressurized = entity->getValue("pressurized1", true);
                bool chamber2Pressurized = entity->getValue("pressurized2", true);
                double m = entity->getValue("mass", 500);
                double Fext = entity->getValue("force", 0.0);
                double d = entity->getValue("damping", 500.0);
                auto pipe1 = e1->pipe.lock();
                auto pipe2 = e2->pipe.lock();
                double rho = pipe1->fluid.effectiveDensity;

                double v = entity->getValue("pistonSpeed", 0.0);
                double A = entity->getValue("area", 0.0);

                int vID = node->stateID;

                if (chamber1Pressurized) {
                    //addChamberFlow(solver, e1,  Q);
                    addChamberFlow(solver, e1, vID, A);
                } else {
                    // assignBoundaryPressures() already set this to the free-surface head.
                    setDirichlet(solver, e1->stateID, e1->hydraulicHead);
                }

                if (chamber2Pressurized) {
                    //addChamberFlow(solver, e2, -Q);
                    addChamberFlow(solver, e2, vID, -A);
                } else {
                    // assignBoundaryPressures() already set this to the free-surface head.
                    setDirichlet(solver, e2->stateID, e2->hydraulicHead);
                }

                /*double R1 = pipe1->computeEffectiveResistance(e1->flow);
                double R2 = pipe2->computeEffectiveResistance(e2->flow);
                double G1 = 2.0 / max(R1, 1e-9); // half-pipe conductance
                double G2 = 2.0 / max(R2, 1e-9); // half-pipe conductance
                solver.A[e1->stateID][vID] =  A / G1;
                solver.A[e2->stateID][vID] = -A / G2;*/

                double k = rho * gravity * A;
                solver.A[vID][e1->stateID]  = -k;
                solver.A[vID][e2->stateID]  =  k;
                solver.A[vID][vID] =  m/dt + d;
                solver.b[vID]      =  m/dt * v - Fext;
                continue;
            }

            addFlowBalance(solver, ends);
            continue;
        }

        if (entity->is_a("ControlValve")) {
            auto& endsGroup = node->endsGroup;
            auto& endGroups = node->endGroups;

            for (size_t i=0; i<ends.size(); i++) { // handle ends without paths/groups
                if (!endsGroup.count(i)) addDeadEnd(solver, ends[i]);
            }

            for (auto& g : endGroups) {
                addFlowBalance(solver, g.second);
            }
            continue;
        }

        if (entity->is_a("Valve")) {
            if (ends.size() == 2) {
                double x = entity->getValue("state", 0.0);
                if (x < 1e-3) {
                    addDeadEnd(solver, ends[0]);
                    addDeadEnd(solver, ends[1]);
                    continue;
                }
            }

            addFlowBalance(solver, ends);
            continue;
        }

        if (entity->is_a("Junction")) {
            addFlowBalance(solver, ends);
            continue;
        }

        if (entity->is_a("Gauge")) {
            addFlowBalance(solver, ends);
            continue;
        }

        cout << "Warning: unhandled node in solve heads: " << entity->toString() << endl;
        addFlowBalance(solver, ends);
    }

    auto T5 = timer->stop();
    detectHydraulicIslands(solver); // 8%
    auto T6 = timer->stop();

    /*cout << "A:" << endl;
    for (auto& r : solver.A.rows) {
        for (auto& c : r.cells) cout << " (" << c.first << ", " << c.second << ")";
        cout << endl;
    }
    cout << "b:";
    for (auto& c : solver.b) cout << " " << c;
    cout << endl;
    cout << "x:";
    for (auto& c : solver.x) cout << " " << c;
    cout << endl;*/

    bool ok = solveLinearSystem(solver); // 88% best
    //bool ok = solveLinearSystem2(solver); // 91% bad, not stable
    //bool ok = solveLinearSystem3(solver); // promising, but not good enough
    //if (!ok) solveLinearSystem(solver); // test as fallback
    auto T7 = timer->stop();
    //if (!ok) cout << " Error, solveNodeHeads::solveLinearSystem failed!" << endl;
    updateHeads(solver.x);
    auto T8 = timer->stop();

    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (!entity) continue;

        if (entity->is_a("Cylinder")) {
            double A = entity->getValue("area", 0.0);
            double L = entity->getValue("length", 0.0);
            double x = entity->getValue("state", 0.0);
            int vID = node->stateID;

            double v = solver.x[vID];
            double dx = v * dt / L;
            double x_new = x + dx;
            x_new = clamp(x_new, 0.001, 0.999);
            dx = x_new - x;
            v = dx*L/dt;

            entity->set("pistonSpeed", toString(v, 12));
        }

        if (entity->is_a("Pump")) {
            double ns = entity->getValue("newState", 0.0);
            entity->set("state", toString(ns,12));
            //cout << " pump state " << ns << ", ctrol " << c << " dt " << dt << endl;
        }
    }

    auto T9 = timer->stop();

    /*int Ncells = 0;
    for (auto& r : solver.A.rows) {
        Ncells += r.cells.size();
    }
    cout << " Ncells " << Ncells / solver.N << ", solver.N " << solver.N << endl;

    cout << " VRPipeSystem::update " << int(T1/T9*100.0)
            << ", " << int((T2-T1)/T9*100.0)
            << ", " << int((T3-T2)/T9*100.0)
            << ", " << int((T4-T3)/T9*100.0)
            << ", " << int((T5-T4)/T9*100.0)
            << ", " << int((T6-T5)/T9*100.0)
            << ", " << int((T7-T6)/T9*100.0)
            << ", " << int((T8-T7)/T9*100.0)
            << ", " << int((T9-T8)/T9*100.0)
            << endl;*/
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
        double dP = dH * pipe->fluid.effectiveDensity * gravity;

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
        double Aeff = pipe->area * max(pipe->level, 0.01);
        double L = pipe->fluid.effectiveDensity * pipe->length / max(Aeff, 1e-9); // inertance
        double R = pipe->computeEffectiveResistance(Q);
        if (halfLength) { R *= 0.5; L *= 0.5; }

        double dP = dH * pipe->fluid.effectiveDensity * gravity;
        double a = dP/L;
        double loss = R * Q * pipe->fluid.effectiveDensity * gravity / L;

        double dQ = a-loss;
        double maxStep = 2.0 * abs(dP) / L;
        dQ = clamp(dQ, -maxStep, maxStep); // stability
        return Q + dQ*dt;
    };

    auto forceFlow = [&](VRPipeEndPtr e, const VRPipeSegmentPtr& pipe) {
        double dH = pipe->hydraulicHead - e->hydraulicHead;
        double flow = computeFlow(dH, pipe, true);
        e->headFlow = flow;
    };

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();

        if (pipe->pressurized && e1->pressurized && e2->pressurized) {
            double dH = e2->hydraulicHead - e1->hydraulicHead;
            double flow = accellerateFlow(dH, pipe, e1->flow);
            e1->headFlow =  flow;
            e2->headFlow = -flow;
            //cout << "accellerate dH " << dH << endl;
            continue;
        }

        if (pipe->pressurized) {
            double dH1 = pipe->hydraulicHead - e1->hydraulicHead;
            double dH2 = pipe->hydraulicHead - e2->hydraulicHead;
            double flow1 = accellerateFlow(dH1, pipe, e1->flow);
            double flow2 = accellerateFlow(dH2, pipe, e2->flow);
            e1->headFlow = flow1;
            e2->headFlow = flow2;
            continue;
        }

        forceFlow(e1, pipe);
        forceFlow(e2, pipe);
    }

    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            double v = entity->getValue("pistonSpeed", 0.0);
            double A = entity->getValue("area", 0.0);
            double hflow = A*v;
            entity->set("headFlow", toString(hflow,12));
            //cout << " cylinder flow: " << hflow << endl;
        }
    }
}

void VRPipeSystem::computeMaxFlows(double dt) {
    double eps = 1e-11;
    auto sign = [](double v) { return (v > 0) - (v < 0); };

    //auto clampFlow = [&](double& flow, double c) -> double {
    auto clampFlow = [&](const VRPipeEndPtr& e, double flow) {
        double f = abs(e->maxFlow);
        e->maxFlow = flow;
        double c = 0.0;
        if (f > 1e-6) c = clamp(1.0 - abs(flow) / f, 0.0, 1.0);
        e->flowClamp = max(c, e->flowClamp);
    };

    //cout << "computeMaxFlows" << endl;
    auto computeContainerFlowScaling = [&](double volAir, double volWater, double flowIn, double flowOut, bool pressurized) {
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

    auto clampCylinderFlows = [&](VRPipeEndPtr& e, double cVolume, double cLevel, double& pistonFlow, int pfSign, bool chamberPressurized) {
        bool pfChanged = false;
        // pistonFlow < 0: piston makes chamber smaller
        // pistonFlow > 0: piston makes chamber bigger

        double vAir = cVolume * (1.0-cLevel);
        double vWat = cVolume * cLevel;
        double flow = e->maxFlow;
        // flow < 0: water leaves the chamber
        // flow > 0: water enters the chamber

        double deltaWaterVol = flow*dt;
        double relPistonFlow = pfSign*pistonFlow;
        double deltaPistonVol = relPistonFlow*dt;

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

        if (chamberPressurized) {
            if (relPistonFlow > 0) { // chamber grows
                if (flow > 0) { // flow pushes, should be same as pistonflow
                    if (flow+eps < relPistonFlow) {
                        pistonFlow = pfSign*flow; pfChanged = true;
                    } // piston too fast
                    if (flow > pistonFlow) {
                        flow = relPistonFlow; // too much flow in
                    }
                }
            }

            if (relPistonFlow < 0) { // chamber shrinks
                if (flow > 0) { // flow pushes against
                    flow = 0;
                    pistonFlow = 0;
                    pfChanged = true;
                }
            }

            // shrinking + flowing out more
        }

        // test too much push out
        double maxPistonFlow = (vAir - deltaWaterVol)/dt;
        if (-pfSign*pistonFlow > maxPistonFlow+eps) { pistonFlow = -pfSign*maxPistonFlow; pfChanged = true; }

        //if (abs(e->maxFlow) > 1e-3) cout << " clampCylinderFlows f0: " << e->maxFlow << " -> " << flow << ", " << flow /  e->maxFlow << endl;
        //if (abs(pistonFlow) > 1e-3) cout << " clampCylinderFlows fc: " << pistonFlow << ", pfSign: " << pfSign << endl;
        //if (hflow > 0 && e1->pressurized) hflow = min(flow1, hflow);
        //if (hflow < 0 && e2->pressurized) hflow =-min(flow2,-hflow);

        /*if (cVolume < 1e-5)
        cout << e->stateID
            << " f " << e->maxFlow << " -> " << flow
            << " cV " << cVolume
            << " cLvl " << cLevel
            << endl;*/
        clampFlow(e, flow);
        return pfChanged;
    };

    auto rebalanceEndsGroupFlow = [&](const vector<VRPipeEndPtr>& ends, double volAir, double volWater) {
        double totalFlowIn = 0;
        double totalFlowOut = 0;
        for (auto& e : ends) {
            auto f = e->maxFlow;
            if (f < 0) totalFlowOut += -f;
            else totalFlowIn += f;
        }

        Vec2d scaleFlowInOut = computeContainerFlowScaling(volAir, volWater, totalFlowIn, totalFlowOut, true);

        for (auto& e : ends) {
            auto f = e->maxFlow;
            if (f < 0) clampFlow(e, f * scaleFlowInOut[1]);
            else clampFlow(e, f * scaleFlowInOut[0]);
        }
    };

    auto processNodes = [&]() {
        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;

            if (entity->is_a("Cylinder")) {
                if (node->pipes.size() != 2) continue;

                double cArea = entity->getValue("area", 1.0);
                double cLength = entity->getValue("length", 1.0);
                double cVolume = cArea * cLength;
                double cLevel1 = entity->getValue("level1", 1.0);
                double cLevel2 = entity->getValue("level2", 1.0);
                bool chamber1Pressurized = entity->getValue("pressurized1", true);
                bool chamber2Pressurized = entity->getValue("pressurized2", true);
                double hflow = entity->getValue("headFlow", 0.0);
                double cState = entity->getValue("state", 0.0);
                double cVol1 = cVolume * cState;
                double cVol2 = cVolume * (1.0-cState);

                auto& e1 = node->pipes[0];
                auto& e2 = node->pipes[1];

                bool changed1 = true;
                bool changed2 = true;
                for (int i=0; (changed1 || changed2) && i < 10; i++) {
                    changed1 = clampCylinderFlows(e1, cVol1, cLevel1, hflow,  1, chamber1Pressurized);
                    changed2 = clampCylinderFlows(e2, cVol2, cLevel2, hflow, -1, chamber2Pressurized);
                }

                // forbid cavitations
                if (chamber1Pressurized && chamber2Pressurized) {
                    double Qp = abs(hflow);
                    double totalFlowIn = 0;
                    double totalFlowOut = 0;
                    for (auto& e : {e1, e2}) {
                        auto f = e->maxFlow;
                        if (f < 0) totalFlowOut += -f;
                        else totalFlowIn += f;
                    }

                    if (totalFlowOut > Qp) { // more flow out than piston flow, clamp flow out!
                        for (auto& e : {e1, e2}) {
                            auto f = e->maxFlow;
                            if (f < 0) clampFlow(e, -Qp);
                        }
                    }
                }

                entity->set("headFlow", toString(hflow,12));
                continue;
            }

            if (entity->is_a("Tank")) {
                double tankArea = entity->getValue("area", 0.0);
                double tankHeight = entity->getValue("height", 0.0);
                double tankVolume = tankHeight * tankArea;
                double tankLevel = entity->getValue("level", 1.0);
                double nodeAirVolume = tankVolume * (1.0-tankLevel);
                double nodeWaterVolume = tankVolume * tankLevel;
                rebalanceEndsGroupFlow(node->pipes, nodeAirVolume, nodeWaterVolume);

                bool tankPressurized = entity->getValue("pressurized", false);
                if (tankPressurized) {
                    double totalFlowIn = 0;
                    double totalFlowOut = 0;
                    for (auto& e : node->pipes) {
                        auto f = e->maxFlow;
                        if (f < 0) totalFlowOut += -f;
                        else totalFlowIn += f;
                    }

                    if (totalFlowOut > abs(totalFlowIn)) { // more flow out than flow in, clamp flow out!
                        double s = abs(totalFlowIn) / totalFlowOut;
                        for (auto& e : node->pipes) {
                            auto f = e->maxFlow;
                            if (f < 0) clampFlow(e, f*s);
                        }
                    }
                }
                continue;
            }

            if (entity->is_a("Valve") || entity->is_a("Pump")) { // ControlValve is also a Valve
                vector<VRPipeEndPtr>& ends = node->pipes;
                auto& endsGroup = node->endsGroup;
                auto& endGroups = node->endGroups;


                for (size_t i=0; i<ends.size(); i++) { // handle ends without paths/groups
                    if (!endsGroup.count(i)) {
                        auto& e = ends[i];
                        clampFlow(e, 0.0);
                    }
                }

                for (auto& g : endGroups) {
                    rebalanceEndsGroupFlow(g.second, 0.0, 0.0);
                }
                continue;
            }

            rebalanceEndsGroupFlow(node->pipes, 0.0, 0.0);
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

            Vec2d scaleFlowInOut = computeContainerFlowScaling(pipeAirVolume, pipeWaterVolume, totalFlowIn, totalFlowOut, pipe->pressurized);

            for (auto& e : {e1, e2}) {
                auto f = e->maxFlow;
                if (f < 0) clampFlow(e, f * scaleFlowInOut[0]);
                else clampFlow(e, f * scaleFlowInOut[1]);
                if (abs(e->maxFlow-f) > 1e-7) needsIteration = true;
            }

            // forbid cavitations
            totalFlowIn = 0;
            totalFlowOut = 0;
            for (auto& e : {e1, e2}) {
                auto f = e->maxFlow;
                if (f < 0) totalFlowIn += -f;
                else totalFlowOut += f;
            }

            if (totalFlowOut > totalFlowIn) {
                if (e1->pressurized && e2->pressurized) {
                    for (auto& e : {e1, e2}) {
                        if (e->maxFlow > 0) clampFlow(e, totalFlowIn);
                    }
                    needsIteration = true;
                } else {
                    if (e1->pressurized && e1->hydraulicHead > e2->hydraulicHead) {
                        if (e2->maxFlow > 0) clampFlow(e2, totalFlowIn);
                        needsIteration = true;
                    }
                    if (e2->pressurized && e2->hydraulicHead > e1->hydraulicHead) {
                        if (e1->maxFlow > 0) clampFlow(e1, totalFlowIn);
                        needsIteration = true;
                    }
                }
            }
        }
    };

    auto computeOrificePressureLoss = [&](double A, double Q, double rho) {
        double v = Q/max(A, 1e-12);
        return 0.5 * rho * v*v * sign(Q);
    };

    auto copyInitialMaxHead = [&]() {
        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;

            for (size_t i=0; i<node->pipes.size(); i++) {
                auto& e = node->pipes[i];
                auto pipe = e->pipe.lock();
                e->flowClamp = 0.0;
                e->maxFlow = e->headFlow;

                if (entity->is_a("Tank")) {
                    double tankLevel = entity->getValue("level", 1.0);
                    double tankHeight = entity->getValue("height", 1.0);
                    auto nPos = graph->getPosition(n.first)->pos();
                    double fluidHeight = (tankLevel-0.5)*tankHeight + nPos[1];
                    if (e->height > fluidHeight && e->headFlow < 0) {
                        clampFlow(e, 0);
                        continue; // pipe end above water level cant drain tank!
                    }
                }

                if (entity->is_a("Valve") || entity->is_a("Pump")) { // includes ControlValve
                    if (!node->pathOpenings.count(i)) {
                        clampFlow(e, 0);
                        continue;
                    }

                    if (node->pathOpenings.count(i)) {
                        double state = node->pathOpenings[i];
                        double Q = e->flow;
                        double rho = pipe->fluid.effectiveDensity;

                        double Apipe = pipe->area * max(pipe->level, 0.01);
                        double Avalve = pipe->area * max(min(pipe->level,state), 0.01);

                        double dPvalve = computeOrificePressureLoss(Avalve, Q, rho);
                        double dPopen  = computeOrificePressureLoss(Apipe,  Q, rho);

                        double L = rho * pipe->length / max(Apipe, 1e-9);
                        double valveAccel = (dPvalve - dPopen) / L;

                        double dQ = (e->headFlow - Q)/dt;
                        dQ -= valveAccel;

                        clampFlow(e, Q + dQ*dt);
                        continue;
                    }
                }
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

    auto rescaleFlowClamp = [&]() {
        double mfc = 0.0;
        for (auto& n : nodes) {
            for (auto& e : n.second->pipes) {
                mfc = max(mfc, e->flowClamp);
            }
        }
        if (mfc < 1e-12) return;

        for (auto& n : nodes) {
            for (auto& e : n.second->pipes) {
                e->flowClamp /= mfc;
            }
        }
    };

    auto checkNodeFlows = [&]() {
        for (auto& n : nodes) {
            auto e = n.second->entity;
            if (e->is_a("Tank")) continue;
            if (e->is_a("Cylinder")) continue;

            //double eps = 1e-15;
            double eps = 1e-4;

            double Q = 0;
            for (auto& e : n.second->pipes) Q += e->maxFlow;
            if (abs(Q) > eps) {
                cout << " junction flow: " << Q << ", " << n.first << ", " << e->getConceptList() << endl;
                for (auto& e : n.second->pipes) {
                    auto p = e->pipe.lock();
                    cout << "  e: " << e->maxFlow << ", R: " << p->radius << ", h: " << e->hydraulicHead << endl;
                }
            }
        }
    };


    copyInitialMaxHead();

    bool needsIteration = true;
    int Nitr = 30;
    for (int i=0; needsIteration && i<Nitr; i++) {
        needsIteration = false;
        processNodes();
        processSegments(needsIteration);

        if (i >= Nitr-1 && needsIteration) {
            //cout << "Warning, not enought iterations: " << i << ", ni " << needsIteration << endl;
            //checkNodeFlows();
        }
    }

    copyFinalMaxHead();
    rescaleFlowClamp();
    //checkNodeFlows();
}

void VRPipeSystem::updateLevels(double dt) {
    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

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
            double newLevel = tankLevel + volDelta / tankVolume;

            newLevel = clamp(newLevel, 0.0, 1.0, true, "tank lvl");
            volDelta = (newLevel - tankLevel)*tankVolume;
            entity->set("level", toString(newLevel,12));

            // update pressurized tank
            if (!tankOpen) {
                double oldVolume = (1.0-tankLevel)*tankVolume;
                double tankPressure = entity->getValue("pressure", atmosphericPressure);
                double initialGasVolume = entity->getValue("initialGasVolume", oldVolume); // avoid drift
                double initialGasPressure = entity->getValue("initialGasPressure", tankPressure); // avoid drift

                double newVolume = (1.0-newLevel) *tankVolume;
                double p = initialGasPressure * initialGasVolume / newVolume;
                /*cout << " tank: " << n.first
                    << " p: " << tankPressure << " p/pI: " << p/initialGasPressure
                    << " level: " << newLevel << " tV " << tankVolume
                    << " e: " << entity
                    << " iGV: " << initialGasVolume << " nV: " << newVolume
                    << endl;*/

                p = clamp(p, atmosphericPressure * 0.001, atmosphericPressure * 2000.0, false, "tank pressure");
                entity->set("pressure", toString(p, 12));
                //cout << " new tank pressure: " << p << endl;
            }
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];

            // compute piston movement
            double x = entity->getValue("state", 0.0);
            double L = entity->getValue("length", 0.0);
            double A = entity->getValue("area", 0.0);
            double dflow = entity->getValue("headFlow", 0.0);
            double vol = L*A;
            double dx = dflow * dt / vol;

            double x_new = x + dx;
            x_new = clamp(x_new, 0.001, 0.999);
            dx = x_new - x;

            double v = dx*L/dt;
            entity->set("state", toString(x_new,12));
            entity->set("pistonSpeed", toString(v,12));
            //cout << " cyl speed " << v << endl;

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
            double newLevel1 = clamp(lvl1, 0.0, 1.0, true, "cylinder lvl1");
            double newLevel2 = clamp(lvl2, 0.0, 1.0, true, "cylinder lvl2");

            /*if (newLevel2 < 0.9 && x < 0.9 && x > 0.1) {
                cout << endl;
                cout << " level cV: " << vol << ", x: " << x << " -> " << x_new << endl;
                cout << " level flows f1: " << e1->flow << ", fc: " << dflow << ", f2: " << e2->flow << endl;
                cout << " level dVw1: " << e1->flow*dt << ", dVc: " << dflow*dt << ", dVw2: " << e2->flow*dt << endl;
                cout << " level V1: " << _vol1 << " -> " << vol1 << ", V2: " << _vol2 << " -> " << vol2 << endl;
                cout << " level wV1: " << _volWater1 << " -> " << volWater1 << ", wV2: " << _volWater2 << " -> " << volWater2 << endl;
                cout << " level wV1+cf: " << _volWater1 << " -> " << _volWater1+dflow*dt << ", wV2-cf: " << _volWater2 << " -> " << _volWater2-dflow*dt << endl;
                cout << " level lvl1: " << _lvl1 << " -> " << lvl1 << ",  lvl2: " << _lvl2 << " -> " << lvl2 << endl;
                cout << " level lvl1-1: " << _lvl1-1.0 << " -> " << lvl1-1.0 << ",  lvl2-1: " << _lvl2-1.0 << " -> " << lvl2-1.0 << endl;
                cout << " level max dflow: " << (_vol2*(1.0-_lvl2) - e2->flow*dt)/dt << ", dflow: " << dflow << endl;
                //cout << "    " << volWater2 / vol2 - 1.0 << " " << vol2 - volWater2 << endl;
            }*/

            entity->set("level1", toString(newLevel1,12));
            entity->set("level2", toString(newLevel2,12));

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
        double volDelta = flow*dt;
        double lvl = clamp(pipe->level -  volDelta / pipe->volume, 0.0, 1.0, true);
        pipe->setLevel(lvl);
    }
}

void VRPipeSystem::updatePressurization(double dt) {
    auto copyUnpressurized = [&](vector<VRPipeEndPtr>& ends) {
        bool isP = true;
        for (auto& e : ends) if (!e->pressurized) isP = false;

        if (!isP) {
            double fluidLvlMin = 1e6;
            for (auto& e : ends) {
                auto p = e->pipe.lock();
                if (p->pressurized) fluidLvlMin = min(fluidLvlMin, p->fluidMax);
                else fluidLvlMin = min(fluidLvlMin, p->fluidLvl);
            }

            for (auto& e : ends) { // TODO: do I realy need this??
                //if (e->heightMax > fluidLvlMin + 1e-6) e->pressurized = false;
            }
        }
    };

    auto checkChamberPressure = [](VREntityPtr entity, double level, string attribute) {
        bool cP = entity->getValue(attribute, true);
        if ( cP && level < 0.98) cP = false;
        if (!cP && level > 0.99) cP = true;
        //cP = bool(level > 0.9999);
        entity->set(attribute, toString(cP));
    };

    auto checkPipeEndPressure = [](VRPipeEndPtr e, VREntityPtr entity, double level, double y0, double H, string chamberPAttrib) {
        bool cP = entity->getValue(chamberPAttrib, true);
        double fluidHeight = (level-0.5)*H + y0;
        if (!cP && e->height > fluidHeight) e->pressurized = false;
    };

    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

        for (auto& pEnd : node->pipes) pEnd->pressurized = true;

        if (entity->is_a("Tank")) {
            // update pipes not submerged by fluid level
            double tankHeight = entity->getValue("height", 0.0);
            double level = entity->getValue("level", 0.0);
            bool tankOpen = entity->getValue("isOpen", false);
            auto nPos = graph->getPosition(n.first)->pos();
            double fluidHeight = (level-0.5)*tankHeight + nPos[1];

            if (tankOpen) entity->set("pressurized", toString(false));
            else checkChamberPressure(entity, level, "pressurized");

            for (auto& e : node->pipes) {
                if (e->height > fluidHeight) e->pressurized = false;
            }
            continue;
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];
            double level1 = entity->getValue("level1", 0.0);
            double level2 = entity->getValue("level2", 0.0);
            double A = entity->getValue("area", 0.0);
            double chamberHeight = sqrt(A);
            auto nPos = graph->getPosition(n.first)->pos();

            checkChamberPressure(entity, level1, "pressurized1");
            checkChamberPressure(entity, level2, "pressurized2");

            checkPipeEndPressure(e1, entity, level1, nPos[1], chamberHeight, "pressurized1");
            checkPipeEndPressure(e2, entity, level2, nPos[1], chamberHeight, "pressurized2");
            continue;
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        //cout << " updatePressurization pipe " << s.first << endl;

        // hysteresis
        if ( pipe->pressurized && pipe->level < 0.95) pipe->pressurized = false;
        if (!pipe->pressurized && pipe->level > 0.98) pipe->pressurized = true;

        if (!pipe->pressurized) {
            for (auto& e : {e1,e2}) {
                if (e->heightMax > pipe->fluidLvl) e->pressurized = false; // ISSUE: accumulator doesnt work anymore..
                //if (e->height > pipe->fluidLvl+1e-3) e->pressurized = false; // old
                //cout << "  end " << e->nID << " P: " << e->pressurized << " eH " << e->height << ", fL " << pipe->fluidLvl << endl;
            }
        }
    }

    for (auto n : nodes) { // second pass for nodes
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Pump")) { // TODO: this is disabled to avoid draining the feed line
            //for (auto& g : node->endGroups) copyUnpressurized(g.second); // only unpressurize if connecting path
            continue;
        }

        if (entity->is_a("Valve")) { // also applies to ControlValve
            for (auto& g : node->endGroups) copyUnpressurized(g.second); // only unpressurize if connecting path
            continue;
        }

        if (entity->is_a("Cylinder")) {
            if (node->pipes.size() != 2) continue;
            auto& e1 = node->pipes[0];
            auto& e2 = node->pipes[1];
            if (!e1->pressurized) entity->set("pressurized1", toString(false));
            if (!e2->pressurized) entity->set("pressurized2", toString(false));
            continue;
        }

        // junctions
        copyUnpressurized(node->pipes);
    }
}

void VRPipeSystem::updatePressures(double dt) {
    for (auto& n : nodes) {
        for (auto& e : n.second->pipes) {
            auto pipe = e->pipe.lock();
            double Pgauge = (e->hydraulicHead - e->height) * pipe->fluid.effectiveDensity * gravity;
            e->pressure = Pgauge + atmosphericPressure;
            /*if (n.first == 4)
            cout << " updatePressure: " << n.first << " " << e
                << " h: " << e->height
                << " H: " << e->hydraulicHead
                << " P: " << e->pressure
                << " rho: " << pipe->fluid.effectiveDensity
                << endl;*/
        }

        if (n.second->userCb) {
            auto e = n.second->entity;
            if (e && e->is_a("Gauge")) {
                auto te = e->getEntity("tank");
                double maxPressure = e->getValue("maxPressure", 10*atmosphericPressure);
                double pressure = e->getValue("pressure", 0.0) - atmosphericPressure;
                if (te && maxPressure > 1e-3) {
                    double pt = te->getValue("pressure", atmosphericPressure) - atmosphericPressure;
                    double indicator = pt/maxPressure;
                    if (abs(pt - pressure)>1e-3) {
                        //cout << "updatePressures gauge " << pt << "/" << maxPressure << ", " << indicator << endl;
                        e->set("pressure", toString(pt,12));
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
        auto mat = materials[pipe->materialID];
        pipe->regime = pipe->computeRegime(Q);
        pipe->updateResistance(mat->friction);
   }
}

void VRPipeSystem::computeFlowMixing(double dt) {

    auto mixNodeFlows = [&](vector<VRPipeEndPtr> ends) {
        int N = ends.size();
        if (N == 0) return;

        const double eps = 1e-15;

        double Qin = 0;
        for (auto e : ends) if (e->flow >= eps) Qin += e->flow;
        if (abs(Qin) < eps) return;

        VRFluidComposition fluid;
        double K = 0.0;

        for (auto& e : ends) {
            if (e->flow < eps) continue; // ignore outgoing flows, only mix whats coming in
            auto pipe = e->pipe.lock();
            double k = e->flow / Qin;
            K += k;
            fluid.mixIn(pipe->fluid, k/K);
        }

        for (auto& e : ends) if (e->flow < 0.0) e->fluid = fluid;
    };

    struct FluidVolume {
        double V;
        VRFluidComposition fluid;
    };

    auto mixVolumeFlows = [&](FluidVolume f0, vector<FluidVolume> flows) {
        if (flows.size() == 0) return f0.fluid;
        if (abs(f0.V) < 1e-12) return f0.fluid; // empty volume has no fl. comp.

        double Vdelta = 0.0;
        for (auto f : flows) Vdelta += f.V;
        double Vbefore = f0.V - Vdelta;

        VRFluidComposition fluid0 = f0.fluid;
        double K = Vbefore;
        for (auto& f : flows) {
            K += f.V;
            if (abs(K) < 1e-12) continue;
            fluid0.mixIn(f.fluid, f.V/K);
        }

        return fluid0;
    };

    auto mixAtNode = [&](VRPipeNodePtr node) {
        auto e = node->entity;

        if (e->is_a("Tank")) {
            double tankArea = e->getValue("area", 0.0);
            double tankHeight = e->getValue("height", 0.0);
            double tankLevel = e->getValue("level", 1.0);
            double tankVolume = tankHeight * tankArea;
            double V0 = tankVolume * tankLevel;
            VREntityPtr fe = e->getEntity("fluid");

            VRFluidComposition tFluid;
            tFluid.fromEntity( fe );

            vector<FluidVolume> flows;
            for (auto e : node->pipes) {
                double V = e->flow * dt;
                if (abs(V) < 1e-9) continue;

                auto pipe = e->pipe.lock();
                if (e->flow < 0.0) flows.push_back({V, tFluid});
                else flows.push_back({V, pipe->fluid});
            }

            tFluid = mixVolumeFlows({V0, tFluid}, flows);
            bool addedBin = tFluid.toEntity( fe );
            if (addedBin) rebuildMesh = true;
            for (auto e : node->pipes) if (e->flow < 0.0) e->fluid = tFluid;
            return;
        }

        if (e->is_a("Valve") || e->is_a("Pump")) {
            auto& endGroups = node->endGroups;
            for (auto& g : endGroups) mixNodeFlows(g.second);
            return;
        }

        if (e->is_a("Junction")) mixNodeFlows(node->pipes);

        // TODO: handle other nodes
    };

    auto mixPipeSegment = [&](VRPipeSegmentPtr pipe) {
        //cout << "segment" << endl;
        auto end1 = pipe->end1.lock();
        auto end2 = pipe->end2.lock();

        double V0 = pipe->level * pipe->volume;

        vector<FluidVolume> flows;
        for (auto e : {end1, end2}) {
            double V = -e->flow * dt;
            if (abs(V) < 1e-9) continue;

            if (e->flow >= 0.0) flows.push_back({V, pipe->fluid});
            else flows.push_back({V, e->fluid});
        }

        pipe->fluid = mixVolumeFlows({V0, pipe->fluid}, flows);
        for (auto e : {end1, end2}) if (e->flow >= 0.0) e->fluid = pipe->fluid;
    };


    for (auto& n : nodes) {
        mixAtNode( n.second );
    }

    for (auto& s : segments) {
        mixPipeSegment( s.second );
    }
}

void VRPipeSystem::radiateHeat(double dt) {
    double cWtr = 4200.0;
    double cAir = 1000.0;
    double rhoAir = 1.2; // kg/m3

    for (auto s : segments) {
        auto& pipe = s.second;
        auto envID = pipe->environmentID;
        auto env = environments[envID];

        double A = 2*Pi*pipe->radius*pipe->length;
        double V = pipe->volume*pipe->level;
        double mWtr = pipe->fluid.effectiveDensity * V;

        auto mat = materials[pipe->materialID];
        double U = mat->thermalConductance;

        double Tenv = env->temperature;
        double mAir = env->volume * rhoAir;
        double T = pipe->fluid.temperature;
        double Q = U * A * (Tenv - T) * dt;

        if (mWtr < 1e-6) continue;
        if (mAir < 1e-6) continue;

        pipe->fluid.temperature += Q/mWtr/cWtr;
        env->temperature -= Q/mAir/cAir;
    }

    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (!entity || !entity->is_a("Tank")) continue;

        VREntityPtr fe = entity->getEntity("fluid");
        if (!fe) continue;

        VRFluidComposition fluid;
        fluid.fromEntity(fe);

        double tankArea   = entity->getValue("area", 0.0);
        double tankHeight = entity->getValue("height", 0.0);
        double tankLevel  = entity->getValue("level", 1.0);
        double tankOpen   = entity->getValue("isOpen", 1.0);
        double envID      = entity->getValue("environmentID", 0);
        double matID      = entity->getValue("materialID", 0);

        auto env = environments[envID];
        auto mat = materials[matID];

        double V = tankArea * tankHeight * tankLevel;
        double mWtr = fluid.effectiveDensity * V;
        double mAir = env->volume * rhoAir;
        if (mAir < 1e-6) continue;
        if (mWtr < 1e-6) continue;

        double perimeter = 4 * sqrt(max(tankArea, 0.0));
        double hWet = tankHeight * tankLevel;
        double A = perimeter * hWet + tankArea;
        if (!tankOpen && tankLevel > 0.99) A += tankArea; // add top if tank closed and full


        double U = mat->thermalConductance;
        double Q = U * A * (env->temperature - fluid.temperature) * dt;

        fluid.temperature += Q / (mWtr * cWtr);
        env->temperature  -= Q / (mAir * cAir);
        fluid.toEntity(fe);
    }
}

void VRPipeSystem::updateThermalDependencies(double dt) {
    for (auto s : segments) {
        auto& pipe = s.second;
        pipe->fluid.updateThermalDependencies();
    }
}

void VRPipeSystem::update() {
    int subSteps = 4;
    double dT = 1.0/60;
    dT *= timeScale;
    double dt = dT/subSteps;

    //sleep(1);


    updateNodePaths();

    for (int i=0; i<subSteps; i++) {
        auto t1 = VRTimer::create();
        updatePressurization(dt);
        auto T1 = t1->stop();
        assignBoundaryPressures(dt);
        auto T2 = t1->stop();
        solveNodeHeads(dt); // 80%
        auto T3 = t1->stop();
        computeHeadFlows(dt);
        auto T4 = t1->stop();
        computeMaxFlows(dt); // 15%
        auto T5 = t1->stop();
        updateLevels(dt);
        auto T6 = t1->stop();
        updatePressures(dt);
        auto T7 = t1->stop();
        computeFlowMixing(dt); // 1%
        auto T8 = t1->stop();
        radiateHeat(dt);
        auto T9 = t1->stop();
        updateThermalDependencies(dt);
        auto T10 = t1->stop();
        updateRegimes(dt);
        auto T11 = t1->stop();
        /*cout << " VRPipeSystem::update " << T1
            << ", " << T2-T1
            << ", " << T3-T2
            << ", " << T4-T3
            << ", " << T5-T4
            << ", " << T6-T5
            << ", " << T7-T6
            << ", " << T8-T7
            << ", " << T9-T8
            << ", " << T10-T9
            << ", " << T11-T10
            << endl;*/
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
