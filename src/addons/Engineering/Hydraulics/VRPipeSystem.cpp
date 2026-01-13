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

VRPipeEnd::VRPipeEnd(VRPipeSegmentPtr s, double h) { pipe = s; height = h; }
VRPipeEnd::~VRPipeEnd() {}
VRPipeEndPtr VRPipeEnd::create(VRPipeSegmentPtr s, double h) { return VRPipeEndPtr( new VRPipeEnd(s,h) ); }



// Pipe Segment ----

VRPipeSegment::VRPipeSegment(int eID, double radius, double length, double level) : eID(eID), radius(radius), length(length), level(level) {
    computeGeometry();
}

VRPipeSegment::~VRPipeSegment() {}

VRPipeSegmentPtr VRPipeSegment::create(int eID, double radius, double length, double level) { return VRPipeSegmentPtr( new VRPipeSegment(eID, radius, length, level) ); }

void VRPipeSegment::setLength(double l) {
    length = l;
    computeGeometry();
}

void VRPipeSegment::computeGeometry() {
    area = Pi * pow(radius,2);
    volume = area * length;
    resistance = 8 * viscosity * length / (Pi * pow(radius,4));
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
    m->addPass();
    m->setPointSize(10);
    m->setLit(0);
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

int VRPipeSystem::addSegment(double radius, int n1, int n2, double level, double h1, double h2) {
    rebuildMesh = true;
    int sID = graph->connect(n1, n2);
    auto p1 = graph->getPosition(n1)->pos();
    auto p2 = graph->getPosition(n2)->pos();
    double length = (p2-p1).length();
    auto s = VRPipeSegment::create(sID, radius, length, level);
    segments[sID] = s;
    auto e1 = VRPipeEnd::create(s, h1);
    auto e2 = VRPipeEnd::create(s, h2);
    s->end1 = e1;
    s->end2 = e2;
    nodes[n1]->pipes.push_back(e1);
    nodes[n2]->pipes.push_back(e2);
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

void VRPipeSystem::printSystem() {
    double totalEnergy = 0;
    for (auto n : nodes) { // print some stats
        auto entity = n.second->entity;
        double P = entity->getValue("pressure", 1.0);
        double A = entity->getValue("area", 1.0);
        double H = entity->getValue("height", 1.0);
        if (entity->is_a("Tank")) cout << " tank (n" << n.first << "): P " << P << " V " << A*H << endl;
        else cout << " " << entity->getName() << " (n" << n.first << ")" << endl;
        for (auto nIn : getInPipes (n.first)) cout << "  in  e" << nIn->eID << endl;
        for (auto nOt : getOutPipes(n.first)) cout << "  out e" << nOt->eID << endl;
        totalEnergy += P*A*H;
    }

    for (auto s : segments) { // print some stats
        auto e1 = s.second->end1.lock();
        auto e2 = s.second->end2.lock();
        double P1 = e1 ? e1->pressure : 0.0;
        double P2 = e2 ? e2->pressure : 0.0;
        double V = s.second->volume;
        cout << " pipe (e" << s.second->eID << "): P " << P1 << "->" << P2 << " \tFl: " << e1->flow << endl;
        totalEnergy += (P1+P2)*0.5*V;
    }
    cout << " total energy: " << totalEnergy << endl;
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
    Pump->addProperty("performance", "double");
    Pump->addProperty("maxPressure", "double");
    Pump->addProperty("isOpen", "bool");
    Outlet->addProperty("radius", "double");
    Outlet->addProperty("pressure", "double");
    Outlet->addProperty("density", "double");
    Valve->addProperty("state", "bool");
    Valve->addProperty("radius", "double");
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
double VRPipeSystem::getPump(string n) { auto e = getEntity(n); return e ? e->getValue("performance", 0.0) : 0.0; }
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

void VRPipeSystem::setPump(string n, double p, double pmax) {
    auto e = getEntity(n);
    if (e) {
        e->set("performance", toString(p));
        e->set("maxPressure", toString(pmax));
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
    string concept = "unknown";
    if (auto c = node->entity->getConcept() ) concept = c->getName();
    labels->add(pn, "Node "+toString(nID)+": "+concept);
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
    const Color3f green(0.2,1.0,0.2);

    VRGeoData data(ptr());

    Vec3d dO = Vec3d(-spread,-spread,-spread);

    if (rebuildMesh) {
        data.reset();
        rebuildMesh = false;
        Vec3d norm(0,1,0);

        for (auto& s : segments) {
            auto edge = graph->getEdge(s.first);

            auto p1 = graph->getPosition(edge.from);
            auto p2 = graph->getPosition(edge.to);

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

                data.pushVert(p1->pos()+dO*k, norm, col1, tcID1);
                data.pushVert(p2->pos()+dO*k, norm, col2, tcID2);
                data.pushLine();
                k++;
            }
        }

        for (auto& n : nodes) {
            auto p = graph->getPosition(n.first);
            auto e = n.second->entity;
            if (e->is_a("Tank")) {
                double s = spread*3;
                data.pushQuad(p->pos(), Vec3d(0,0,1), Vec3d(0,1,0), Vec2d(s,s), true);
                data.pushQuad(p->pos(), Vec3d(0,0,1), Vec3d(0,1,0), Vec2d(s,s), true);
                for (int i=0; i<4; i++) data.pushColor(blue);
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

    int i=0;
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
        }

        //cout << "flow " << s.first << " " << f << endl;
    }

    for (auto& n : nodes) {
        if (n.second->entity->is_a("Tank")) {
            double l = n.second->entity->getValue("level", 0.0);
            double A = 3*spread;
            //c = s ? Color3f(0,1,0) : Color3f(1,0,0);
            Pnt3d pwRef1 = data.getPosition(i+3);
            Pnt3d pwRef2 = data.getPosition(i+2);
            Pnt3d pw1 = pwRef1 - Vec3d(0,l*A,0);
            Pnt3d pw2 = pwRef2 - Vec3d(0,l*A,0);
            data.setPos(i+0, pw1);
            data.setPos(i+1, pw2);
            data.setPos(i+7, pw1);
            data.setPos(i+6, pw2);
            i += 8;
        } else {
            Color3f c(0.4,0.4,0.4);
            if (n.second->entity->is_a("Valve")) {
                bool s = n.second->entity->getValue("state", false);
                c = s ? Color3f(0,1,0) : Color3f(1,0,0);
            }

            if (n.second->entity->is_a("Junction")) c = Color3f(0.2,0.4,1);
            if (n.second->entity->is_a("Outlet"))   c = Color3f(0.1,0.2,0.8);

            if (n.second->entity->is_a("Pump")) {
                double p = n.second->entity->getValue("performance", 0.0);
                c = p>1e-3 ? Color3f(1,1,0) : Color3f(1,0.5,0);
            }

            data.setColor(i, c); i++;
        }
    }
}



/** ---- simulation ---- */



void VRPipeSegment::addEnergy(double m, double d, bool p1, string hint) {
    /*if (volume < 1e-9) return;

    double pressure = p1 ? pressure1 : pressure2;

    if (p1) pressure1 += m/volume;
    else pressure2 += m/volume;

    if (pressure1 < -1e-6 || pressure2 < -1e-6) {
        cout << " hint: " << hint << ", m: " << m << ", mv: " << m/volume << ", p: " << pressure << " -> " << double(p1 ? pressure1 : pressure2) << endl;
        printBacktrace();
    }

    if (m>0) { // only when adding material with different density
        density = (density * volume + m*d) / (volume + m);
    }*/
}

void VRPipeSegment::handleTank(double& otherPressure, double otherVolume, double& otherDensity, double dt, bool p1) {
    /*if (otherVolume < 1e-9) return;

    double pressure = p1 ? pressure1 : pressure2;
    double dP = pressure - otherPressure;
    double m = dP*area*dt*gasSpeed; // energy through the pipe section area

    if (dP > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of tank
        m = min(m, otherPressure*otherVolume); // not more than available!
    }

    addEnergy(-m, otherDensity, p1, "handleTank");
    //if (pressure1 < -1e-6 || pressure2 < -1e-6) cout << "handleTank, m: " << m << " mv: " << m/volume << " dP: " << dP << " P: " << pressure << "->" << otherPressure ;
    otherPressure += m/otherVolume;
    if (m>0) otherDensity = (otherDensity * otherVolume + m*density) / (otherVolume + m); // only when adding material with different density
    //cout << " ... " <<  " P: " << pressure << "->" << pressure1 << "->" << pressure2 << " pipe: " << this << endl;*/
}

double VRPipeSegment::computeExchange(double hole, VRPipeSegmentPtr other, double dt, bool p1, bool op1) {
    /*double pressure = p1 ? pressure1 : pressure2;
    double otherPressure = op1 ? other->pressure1 : other->pressure2;
    double dP = pressure - otherPressure;
    hole = min(hole, min(this->area, other->area));
    double m = dP*hole*dt*gasSpeed; // energy through the opening

    if (m > 0) { // energy is going out of pipe
        m = min(m, pressure*volume); // not more than available!
    } else { // energy going out of other
        m = -min(-m, otherPressure*other->volume); // not more than available!
    }

    //cout << "computeExchange eID: " << eID << " (" << other->eID << ")" << ", dP: " << dP << ", m: " << m << " " << endl;
    return m;*/
    return 0;
}

void VRPipeSegment::handleValve(double area, VRPipeSegmentPtr other, double dt, bool p1, bool op1) {
    /*double m = computeExchange(area, other, dt, p1, op1)*0.5;
    //cout << "handleValve " << m << endl;
    addEnergy(-m, other->density, p1, "handleValveSelf");
    other->addEnergy(m, density, op1, "handleValveOther");*/
}

void VRPipeSegment::handleOutlet(double area, double extPressure, double extDensity, double dt, bool p1) {
    /*double pressure = p1 ? pressure1 : pressure2;
    area = min(area, this->area);
    double dP = pressure - extPressure;
    double m = dP*area*dt*gasSpeed; // energy through the opening
    m = min(m, pressure*volume); // not more than available!
    addEnergy(-m, extDensity, p1, "handleOutlet");*/
}

void VRPipeSegment::handlePump(double performance, double maxPressure, bool isOpen, VRPipeSegmentPtr other, double dt, bool p1, bool op1) {
    /*double pressure = p1 ? pressure1 : pressure2;
    double otherPressure = op1 ? other->pressure1 : other->pressure2;
    if (pressure < 1e-6) return; // min pressure
    if (otherPressure > maxPressure) return;

    double mO = 0;
    if (isOpen) mO = computeExchange(area*0.1, other, dt, p1, op1); // minimal exchange if pump is open

    double v = 1.0 + otherPressure/pressure;
    double m = performance*dt/exp(v);
    m = max(m, mO);
    //if (isOpen) m = max(m, computeExchange(area*0.1, other, dt, p1)); // minimal exchange if pump is open
    m = min(m, pressure*volume); // pump out not more than available!
    addEnergy(-m, other->density, p1, "handlePump");
    other->addEnergy(m, density, op1, "handlePumpOther");
    //cout << " pump " << performance << " m " << m << " v " << v << endl;*/
}



void VRPipeSystem::assignBoundaryPressures() {
    for (auto n : nodes) {
        int nID = n.first;
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Outlet")) {
            double outletRadius = entity->getValue("radius", 0.0);
            if (outletRadius < 1e-6) continue; // closed

            double outletPressure = entity->getValue("pressure", 1.0);
            for (auto& e : node->pipes) e->pressure = outletPressure;
            continue;
        }
    }
}

void VRPipeSystem::computePipePressures(double dt) {
    auto clamp = [](double f, double a = -1, double b = 1) -> double { return f<a ? a : f>b ? b : f; };

    auto computeHydraustaticPressure = [this](const double& height, const double& density) {
        return height * density * gravity;
    };

    for (auto n : nodes) { // traverse nodes, change pressure in segments
        int nID = n.first;
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 0.0);
            double tankHeight = entity->getValue("height", 0.0);
            double tankLevel = entity->getValue("level", 1.0);
            double tankPressure = entity->getValue("pressure", 1.0);
            double tankDensity = entity->getValue("density", 1000.0);
            bool tankOpen = entity->getValue("isOpen", false);

            for (auto& pEnd : node->pipes) {
                double h = max(0.0, tankLevel - pEnd->height) * tankHeight;
                double hP = computeHydraustaticPressure(h, tankDensity);
                if (tankOpen) pEnd->pressure = hP + atmosphericPressure;
                else pEnd->pressure = hP + tankPressure;
                //cout << " tank " << nID << ", " << tankLevel << ", " << pEnd->height << ", " << h << endl;
            }
        }
    }
}

void VRPipeSystem::computePipeFlows(double dt) {
    auto sign = [](double v) { return v < 0 ? -1 : 1; };

    for (auto& s : segments) {
        auto& pipe = s.second;
        auto e1 = pipe->end1.lock();
        auto e2 = pipe->end2.lock();
        double dP = e2->pressure - e1->pressure; // compute pressure gradient

        // based on Darcy-Weisbach for turbulent flow
        double Rt = pipeFriction * pipe->length * pipe->density / ( 4 * pipe->radius * pow(pipe->area,2));
        double flow = sqrt( abs(dP) / Rt ) * sign(dP);

        e1->flow =  flow;
        e2->flow = -flow;
        //cout << "pipe flow " << flow << ", delta P: " << dP << ", R: " << pipe->resistance << " " << endl;
    }
}

void VRPipeSystem::updateLevels(double dt) {
    for (auto n : nodes) { // traverse nodes, change pressure in segments
        int nID = n.first;
        auto node = n.second;
        auto entity = node->entity;

        if (entity->is_a("Tank")) {
            double tankArea = entity->getValue("area", 0.0);
            double tankHeight = entity->getValue("height", 0.0);
            double tankVolume = tankHeight * tankArea;
            double tankLevel = entity->getValue("level", 1.0);
            double tankPressure = entity->getValue("pressure", 1.0);
            double tankDensity = entity->getValue("density", 1000.0);
            bool tankOpen = entity->getValue("isOpen", false);

            double flow = 0;
            for (auto& pEnd : node->pipes) {
                auto pipe = pEnd->pipe.lock();
                if (!pipe) continue;
                flow += pEnd->flow;
                // TODO: limit flow here?
            }

            double level = clamp(tankLevel + flow*dt / tankVolume, 0.0, 1.0);
            //cout << std::setprecision(17);
            //cout << "update tank " << nID << ", level: " << tankLevel << " -> " << level << ", flow: " << flow << ", delta: " << tankLevel + flow*dt / tankVolume << endl;
            entity->set("level", toString(level));
        }
    }
}


void VRPipeSystem::update() {
    int subSteps = 10;
    double dT = 1.0/60;
    double dt = dT/subSteps;

    for (int i=0; i<subSteps; i++) {
        assignBoundaryPressures();
        computePipePressures(dt);
        computePipeFlows(dt);
        updateLevels(dt);
    }

    updateVisual();
}

