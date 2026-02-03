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

VRPipeSegment::VRPipeSegment(int eID, double radius, double length, double level) : eID(eID), radius(radius), length(length), level(level) {
    pressurized = bool(level > 1.0-1e-6);
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
    //resistance = 8 * viscosity * length / (Pi * pow(radius,4));
    resistance = friction * length * density / ( 4 * radius * pow(area,2));
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

    auto P = graph->getPosition(e->nID);
    e->height = (P->pos()+e->offset)[1];
    e->hydraulicHead = e->height;
}

int VRPipeSystem::addSegment(double radius, int n1, int n2, double level, double h1, double h2) {
    rebuildMesh = true;
    int sID = graph->connect(n1, n2);
    auto p1 = graph->getPosition(n1)->pos();
    auto p2 = graph->getPosition(n2)->pos();
    double length = (p2-p1).length();
    auto s = VRPipeSegment::create(sID, radius, length, level);
    segments[sID] = s;
    auto e1 = VRPipeEnd::create(s, n1, h1);
    auto e2 = VRPipeEnd::create(s, n2, h2);
    s->end1 = e1;
    s->end2 = e2;
    nodes[n1]->pipes.push_back(e1);
    nodes[n2]->pipes.push_back(e2);
    computeEndOffset(e1);
    computeEndOffset(e2);
    return sID;
}

void VRPipeSystem::remSegment(int eID) {
    rebuildMesh = true;
    auto& e = graph->getEdge(eID);
    graph->disconnect(e.from, e.to);
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
            double tankArea = entity->getValue("area", 0.0);
            double tankHeight = entity->getValue("height", 0.0);
            double tankVolume = tankArea * tankHeight;
            double tankLevel = entity->getValue("level", 0.0); // 0..1
            double tankDensity = entity->getValue("density", 1000.0); // kg/m³
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
    auto Valve = ontology->addConcept("Valve");

    Tank->addProperty("pressure", "double");
    Tank->addProperty("area", "double");
    Tank->addProperty("height", "double");
    Tank->addProperty("density", "double");
    Tank->addProperty("level", "double");
    Tank->addProperty("isOpen", "bool");
    Pump->addProperty("headGain", "double");
    Pump->addProperty("isOpen", "bool");
    Outlet->addProperty("radius", "double");
    Outlet->addProperty("pressure", "double");
    Outlet->addProperty("density", "double");
    Valve->addProperty("state", "double");
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
double VRPipeSystem::getSegmentDensity(int i) { return segments[i]->density; }
double VRPipeSystem::getSegmentFlow(int i) { auto e1 = segments[i]->end1.lock(); return e1 ? e1->flow : 0; }
double VRPipeSystem::getTankPressure(string n) { auto e = getEntity(n); return e ? e->getValue("pressure", 1.0) : 0.0; }
double VRPipeSystem::getTankDensity(string n) { auto e = getEntity(n); return e ? e->getValue("density", 1000.0) : 0.0; }
double VRPipeSystem::getTankLevel(string n) { auto e = getEntity(n); return e ? e->getValue("level", 1.0) : 0.0; }
double VRPipeSystem::getPump(string n) { auto e = getEntity(n); return e ? e->getValue("headGain", 0.0) : 0.0; }
bool VRPipeSystem::getValveState(string n) { auto e = getEntity(n); return e ? e->getValue("state", false) : false; }

void VRPipeSystem::setValve(string n, bool b)  { auto e = getEntity(n); if (e) e->set("state", toString(b)); }
void VRPipeSystem::setTankPressure(string n, double p) { auto e = getEntity(n); if (e) e->set("pressure", toString(p)); }
void VRPipeSystem::setTankDensity(string n, double p) { auto e = getEntity(n); if (e) e->set("density", toString(p)); }
void VRPipeSystem::setPipeRadius(int i, double r) { segments[i]->radius = r; segments[i]->computeGeometry(); }
void VRPipeSystem::setOutletDensity(string n, double p) { auto e = getEntity(n); if (e) e->set("density", toString(p)); }
void VRPipeSystem::setOutletPressure(string n, double p) { auto e = getEntity(n); if (e) e->set("pressure", toString(p)); }

void VRPipeSystem::setPipePressure(int i, double p1, double p2) {
    auto e1 = segments[i]->end1.lock();
    auto e2 = segments[i]->end2.lock();
    if (e1) e1->pressure = p1;
    if (e2) e2->pressure = p2;
}

void VRPipeSystem::setPump(string n, double h, bool isOpen) {
    auto e = getEntity(n);
    if (e) {
        e->set("headGain", toString(h));
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
    if (!doVisual) {
        VRGeoData data(ptr());
        if (data.size() > 0) {
            data.reset();
            data.apply(ptr());
        }
        return;
    }

    const Color3f white(1,1,1);
    const Color3f yellow(1,1,0);
    const Color3f blue(0.2,0.5,1);
    const Color3f dblue(0.1,0.4,0.7);
    const Color3f green(0.2,1.0,0.2);

    VRGeoData data(ptr());

    Vec3d dO = Vec3d(-spread,-spread,-spread);

    auto updatePipeInds = [&](VRGeoData& data, double level, int i0, int k0) {
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
            auto edge = graph->getEdge(s.first);
            double r = s.second->radius;
            double l = s.second->length;

            Vec3d p1 = graph->getPosition(edge.from)->pos();
            Vec3d p2 = graph->getPosition(edge.to)->pos();

            p1 += s.second->end1.lock()->offset;
            p2 += s.second->end2.lock()->offset;

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
                else continue;

                data.pushVert(p1+dO*k, norm, col1, tcID1);
                data.pushVert(p2+dO*k, norm, col2, tcID2);
                data.pushLine();
                k++;
            }

            if (p2[1] < p1[1]) swap(p1,p2); // p2 always higher than p1
            Vec3d pm = (p1+p2)*0.5;
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

        for (auto& n : nodes) {
            auto p = graph->getPosition(n.first);
            auto e = n.second->entity;
            if (e->is_a("Tank")) {
                double a = e->getValue("area", 0.0);
                double h = e->getValue("height", 0.0);
                double s = sqrt(a);

                data.pushQuad(p->pos() - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(s,s), false);
                data.pushQuad(p->pos() - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(s,s), true);
                data.pushQuad(p->pos() - Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(s,s), false);
                data.pushQuad(p->pos() + Vec3d(0,h*0.5,0), Vec3d(0,1,0), Vec3d(0,0,1), Vec2d(s,s), true);
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
            } else {
                data.pushVert(p->pos(), norm, white, Vec2d());
                data.pushPoint();
            }
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

        // visuals
        Color3f col1, col2;
        for (auto l : layers) {
            if (l == "p") { col1 = c1; col2 = c2; }
            else if (l == "d") { col1 = cd; col2 = cd; }
            else if (l == "v") { col1 = cf; col2 = cf; }
            else if (l == "n") { col1 = white; col2 = green; }
            else continue;

            data.setColor(i, col1); i++;
            data.setColor(i, col2); i++;
            k += 2;
        }

        double l = s.second->level;
        //l = 0.5+0.5*sin(F); // TOTEST

        l = clamp(l, 1e-3, 1.0-1e-3);
        if (abs(l-s.second->lastVizLevel) > 1e-3) {
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

            for (int j=0; j<4; j++) {
                if (!s.second->pressurized) {
                    data.setColor(i+4+j, green);
                    data.setColor(i+8+j, green);
                } else {
                    data.setColor(i+4+j, blue);
                    data.setColor(i+8+j, blue);
                }
            }
        }

        double r = s.second->radius;
        double v0 = 0.5; // m/s
        double v_ref = 4.0; // m/s
        double v = flow/s.second->area;
        double F = 2.0*r*std::asinh(abs(v) / v0) / std::asinh(v_ref / v0);

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

    for (auto& n : nodes) {
        auto e = n.second->entity;
        if (e->is_a("Tank")) {
            double l = e->getValue("level", 0.0);
            double h = e->getValue("height", 0.0);
            double s = h*l;
            for (int j=0; j<4; j++) {
                Pnt3d p = data.getPosition(i+j);
                p[1] += s;
                data.setPos(i+4+j, p);
                data.setPos(i+8+j, p);
            }
            i += 16;
            k += 48;
        } else {
            Color3f c(0.4,0.4,0.4);
            if (e->is_a("Valve")) {
                bool s = e->getValue("state", false);
                c = s ? Color3f(0,1,0) : Color3f(1,0,0);
            }

            if (e->is_a("Junction")) c = Color3f(0.2,0.4,1);
            if (e->is_a("Outlet"))   c = Color3f(0.1,0.2,0.8);

            if (e->is_a("Pump")) {
                double p = e->getValue("performance", 0.0);
                c = p>1e-3 ? Color3f(1,1,0) : Color3f(1,0.5,0);
            }

            data.setColor(i, c); i++;
            k++;
        }

        // hydraulicHead headFlow maxFlow flow
        //string data;
        //for (auto& e : n.second->pipes) data += " " + toString(round(e->hydraulicHead*1000)*0.001);
        //ann->set(n.first, getNodePose(n.first)->pos(), data);
    }
}



/** ---- simulation ---- */

void VRPipeSystem::assignBoundaryPressures() {
    for (auto n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Outlet")) {
            double outletRadius = entity->getValue("radius", 0.0);
            if (outletRadius < 1e-6) continue; // closed

            double outletPressure = entity->getValue("pressure", atmosphericPressure);
            for (auto& e : node->pipes) {
                auto pipe = e->pipe.lock();
                e->hydraulicHead = e->height + outletPressure/pipe->density/gravity;
            }
            continue;
        }

        if (entity->is_a("Tank")) {
            double tankPressure = entity->getValue("pressure", atmosphericPressure);
            double tankDensity = entity->getValue("density", atmosphericPressure);
            double tankHeight = entity->getValue("height", atmosphericPressure);
            double tankLevel = entity->getValue("level", atmosphericPressure);
            bool tankOpen = entity->getValue("isOpen", false);
            if (tankOpen) tankPressure = atmosphericPressure;

            for (auto& e : node->pipes) {
                double depth = max(0.0, tankLevel - e->offsetHeight) * tankHeight;
                e->hydraulicHead = e->height + depth + tankPressure/tankDensity/gravity;
            }
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        double h0 = min(e1->height, e2->height);
        pipe->liquidHead = h0 + pipe->level * pipe->radius * 2;
    }
}

void VRPipeSystem::computeDynamicPipeResistances() {
    double RmaxPipe = 1e12;
    double RmaxTank = 1e12;

    for (auto& s : segments) s.second->dynamicResistance = 0.0;

    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

        auto computeDynamicResistance = [](double opening, double Rmax) {
            return Rmax * (1.0 - opening);
        };

        if (entity->is_a("Pump")) {
            if (node->pipes.size() != 2) continue;
            bool isOpen = entity->getValue("isOpen", 0.0);
            bool headGain = entity->getValue("headGain", 0.0);

            if (headGain < 1e-3) { // pump off
                auto downstream = node->pipes[1];
                auto pipe = downstream->pipe.lock();
                if (!pipe) continue;
                pipe->dynamicResistance += isOpen ? 0 : 1e11;
            }
        }

        if (entity->is_a("Valve")) {
            if (node->pipes.size() != 2) continue;
            double state = entity->getValue("state", 1.0);
            state = clamp(state, 0.0, 1.0);
            auto downstream = node->pipes[1];
            auto pipe = downstream->pipe.lock();
            if (!pipe) continue;
            pipe->dynamicResistance += computeDynamicResistance(state, 1e14);
        }
    }
}

void VRPipeSystem::solveNodeHeads() {
    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank") || entity->is_a("Outlet")) continue; // already prescribed
        if (entity->is_a("Pump")) continue; // done later

        double num = 0.0;
        double den = 0.0;

        for (auto& e : node->pipes) {
            auto pipe = e->pipe.lock();
            auto otherEnd = pipe->otherEnd(e);
            double R = pipe->resistance + pipe->dynamicResistance; // or Rt-equivalent

            if (pipe->pressurized) {
                num += std::max(otherEnd->hydraulicHead, pipe->liquidHead) / R;
                den += 1.0 / R;
            //} else if (pipe->liquidHead > e->hydraulicHead) {
            } else {
                num += pipe->liquidHead / R;
                den += 1.0 / R;
            }
        }

        if (abs(den) < 1e-9) continue;
        double newHead = num / den;
        //cout << " solveNodeHeads " << node->name << "  " << newHead << endl;
        for (auto& e : node->pipes) e->hydraulicHead = newHead;
    }

    for (auto& n : nodes) {
        auto node = n.second;
        auto entity = node->entity;
        if (entity->is_a("Pump")) {
            if (node->pipes.size() != 2) continue;
            double pumpGain = entity->getValue("headGain", 0.0);
            auto pEnd1 = node->pipes[0];
            auto pEnd2 = node->pipes[1];
            double deltaHead = pEnd2->hydraulicHead - pEnd1->hydraulicHead;
            double mod = clamp(pumpGain - deltaHead, 0.0, pumpGain);
            pEnd1->hydraulicHead -= mod*0.5;
            pEnd2->hydraulicHead += mod*0.5;
            //pEnd2->hydraulicHead = pEnd1->hydraulicHead + pumpGain;
        }
    }
}

void VRPipeSystem::computeHeadFlows(double dt) {
    auto sign = [](double v) { return (v > 0) - (v < 0); };

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();

        if (pipe->pressurized) {
            double dH = e2->hydraulicHead - e1->hydraulicHead; // compute hydraulic gradient
            double dP = dH * pipe->density * gravity;
            int dir = sign(dH);

            // based on Darcy-Weisbach for turbulent flow
            //double Rt = pipeFriction * pipe->length * pipe->density / ( 4 * pipe->radius * pow(pipe->area,2));
            double Rt = pipe->resistance + pipe->dynamicResistance;
            double flow = sqrt( abs(dP) / Rt ) * dir;

            e1->headFlow =  flow;
            e2->headFlow = -flow;
        } else {
            // determine upstream end
            auto up   = (e1->hydraulicHead > e2->hydraulicHead) ? e1 : e2;
            auto down = (up == e1) ? e2 : e1;
            double dHfill = up->hydraulicHead - pipe->liquidHead; // driving head relative to pipe liquid surface

            if (dHfill > 0.0) {
                double A = pipe->area;
                double Rt = pipe->resistance + pipe->dynamicResistance;
                double K = (2.0 * A * A * Rt) / pipe->density; // convert quadratic resistance to loss coefficient
                K = std::max(K, 1e-6); // numerical safety
                double flow = A * sqrt(2.0 * gravity * dHfill / K);

                //if (dHfill > 1e-3) cout << " dHfill: " << dHfill << ", flow: " << flow << endl;

                if (up == e1) {
                    e1->headFlow = -flow;   // leaving node
                    e2->headFlow =  0.0;    // no transmission yet
                } else {
                    e1->headFlow =  0.0;
                    e2->headFlow = -flow;
                }
            }
        }
    }
}

void VRPipeSystem::computeMaxFlows(double dt) { // deprecated, used only to copy from headFlow to flow..
    auto computeContainerFlowScaling = [&](string ID, double nodeAirVolume, double nodeWaterVolume, double flowIn, double flowOut) {
        double totalFlow = flowIn - flowOut;
        double maxFlowIn = flowIn;
        double maxFlowOut = flowOut;
        if ( totalFlow*dt > nodeAirVolume)   maxFlowIn  =  min(flowIn,  nodeAirVolume/dt   + flowOut); // node will fill up, clamp flow in
        if (-totalFlow*dt > nodeWaterVolume) maxFlowOut =  min(flowOut, nodeWaterVolume/dt + flowIn); // node will drain, clamp flow out

        double scaleFlowIn  = abs(flowIn)  > 1e-12 ? maxFlowIn  / flowIn  : 1.0;
        double scaleFlowOut = abs(flowOut) > 1e-12 ? maxFlowOut / flowOut : 1.0;
        //cout << "AA " << ID << " " << Vec2d(scaleFlowIn, scaleFlowOut) << ", w: " << nodeWaterVolume << ", dV: " << totalFlow*dt << endl;
        /*if (abs(scaleFlowIn) < 1.0 ||  abs(scaleFlowOut) < 1.0 || abs(scaleFlowIn) > 1.0 ||  abs(scaleFlowOut) > 1.0) {
            cout << "AA " << ID << " " << Vec2d(scaleFlowIn, scaleFlowOut) << ", io: " << Vec2d(flowIn, flowOut) << ", mio: " << Vec2d(maxFlowIn, maxFlowOut) << ", dV: " << totalFlow*dt << endl;
        }*/
        return Vec2d(scaleFlowIn, scaleFlowOut);
    };

    auto processNodes = [&]() {
        for (auto& n : nodes) {
            auto node = n.second;
            auto entity = node->entity;

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

            Vec2d scaleFlowInOut = computeContainerFlowScaling(node->name, nodeAirVolume, nodeWaterVolume, totalFlowIn, totalFlowOut);

            for (auto& e : n.second->pipes) {
                //bool verb = bool(abs(e->maxFlow) > 0);
                //if (verb) cout << " Vair: " << nodeAirVolume << ", Vwater: " << nodeWaterVolume << ", fIn: " << totalFlowIn << ", fOut: " << totalFlowOut;
                //if (verb) cout << ", maxFlow: " << e->maxFlow << " -> ";
                auto f = e->maxFlow;
                if (f < 0) e->maxFlow = f * scaleFlowInOut[1];
                else e->maxFlow = f * scaleFlowInOut[0];
                //if (verb) cout << e->maxFlow << endl;
            }
        }
    };

    auto processSegments = [&](bool& needsIteration) {
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

            Vec2d scaleFlowInOut = computeContainerFlowScaling("seg"+toString(pipe->eID), pipeAirVolume, pipeWaterVolume, totalFlowIn, totalFlowOut);
            if (scaleFlowInOut[0] < 0.9) needsIteration = true;
            if (scaleFlowInOut[1] < 0.9) needsIteration = true;

            for (auto& e : {e1, e2}) {
                auto f = e->maxFlow;
                if (f < 0) e->maxFlow = f * scaleFlowInOut[0];
                else e->maxFlow = f * scaleFlowInOut[1];
            }
        }
    };

    for (auto& n : nodes) {
        for (auto& e : n.second->pipes) {
            e->maxFlow = e->headFlow;
        }
    }

    bool needsIteration = true;
    for (int i=0; needsIteration && i<100; i++) {
        needsIteration = false;
        processNodes();
        processSegments(needsIteration);
    }

    for (auto& n : nodes) {
        for (auto& e : n.second->pipes) {
            e->flow = e->maxFlow;
        }
    }
}

void VRPipeSystem::updateLevels(double dt) {
    for (auto n : nodes) { // traverse nodes, change pressure in segments
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 0.0);
            double tankHeight = entity->getValue("height", 0.0);
            double tankVolume = tankHeight * tankArea;
            double tankLevel = entity->getValue("level", 1.0);

            double totalFlow = 0;
            for (auto& pEnd : node->pipes) {
                auto pipe = pEnd->pipe.lock();
                if (!pipe) continue;
                totalFlow += pEnd->flow; // positive flow is defined as going out the pipe
            }

            double newLevel = clamp(tankLevel + totalFlow*dt / tankVolume, 0.0, 1.0);
            entity->set("level", toString(newLevel));
        }
    }

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        double flow = e1->flow + e2->flow; // positive flow is going out the pipe
        pipe->level = clamp(pipe->level - flow*dt / pipe->volume, 0.0, 1.0);

        // hysteresis
        if ( pipe->pressurized && pipe->level < 0.95) pipe->pressurized = false;
        if (!pipe->pressurized && pipe->level > 0.98) pipe->pressurized = true;
        //pipe->pressurized = bool(pipe->level > 1.0-1e-6);
        //pipe->pressurized = true;
    }
}

void VRPipeSystem::updatePressures(double dt) {
    for (auto& n : nodes) {
        for (auto& e : n.second->pipes) {
            auto pipe = e->pipe.lock();
            e->pressure = e->hydraulicHead * pipe->density * gravity;
        }
    }
}

void VRPipeSystem::update() {
    int subSteps = 10;
    double dT = 1.0/60;
    //dT *= 0.01; // for debugging
    double dt = dT/subSteps;

    //sleep(1);


    for (int i=0; i<subSteps; i++) {
        assignBoundaryPressures();
        computeDynamicPipeResistances();
        solveNodeHeads();
        computeHeadFlows(dt);
            //auto t = VRTimer::create();
        computeMaxFlows(dt); // most time spent
            //cout << "t " << t->stop() << endl;
        updateLevels(dt);
        updatePressures(dt);
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
