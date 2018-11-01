#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"
#include "core/tools/VRPathtool.h"

#include <OpenSG/OSGMatrixUtility.h>

#include "core/scene/VRScene.h"
#include <boost/bind.hpp>

#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {
    updateCb = VRUpdateCb::create("process layout update", boost::bind(&VRProcessLayout::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) {
    auto l = VRProcessLayoutPtr(new VRProcessLayout(name) );
    l->init();
    return l;
}

void VRProcessLayout::init() {
    toolSID = VRPathtool::create();
    toolSID->setPersistency(0);
    addChild(toolSID);
}

VRPathtoolPtr VRProcessLayout::getSIDPathtool() { return toolSID; }
VRPathtoolPtr VRProcessLayout::getSBDPathtool(int subject) { return toolSBDs[subject]; }

/* IDEAS

- one material for all widgets
    - one big texture for all labels


*/

int wrapString(string& s, int width) {
    int w = 0;
    vector<int> newBreaks;
    for (uint i=0; i<s.size(); i++) {
        char c = s[i];

        if (c == '\n') w = 0;
        else w++;

        if (w >= width) {
            newBreaks.push_back(i);
            w = 0;
        }
    }

    for (auto i : newBreaks) s.insert(i, "\n");

    int N = 1;
    for (auto c : s) if (c == '\n') N++;
    return N;
}

Vec4i pushRectVerts(VRGeoData& geo, float x0, float x1, float y0, float y1, Vec3d n, Vec3d u, float d) {
    Matrix4d m;
    MatrixLookAt(m, Pnt3d(0,0,0), n, u);

    Pnt3d p1(x0,y0,0); m.mult(p1,p1);
    Pnt3d p2(x1,y0,0); m.mult(p2,p2);
    Pnt3d p3(x1,y1,0); m.mult(p3,p3);
    Pnt3d p4(x0,y1,0); m.mult(p4,p4);

    int A = geo.pushVert(p1 - n*d, n, Vec2d(0,0));
    int B = geo.pushVert(p2 - n*d, n, Vec2d(1,0));
    int C = geo.pushVert(p3 - n*d, n, Vec2d(1,1));
    int D = geo.pushVert(p4 - n*d, n, Vec2d(0,1));
    return Vec4i(A,B,C,D);
}

void pushLabelFace(VRGeoData& geo, int N, float s, float h1, float h2, Vec3d n, Vec3d u) {
    auto q1 = pushRectVerts(geo, -s, s, -h1, h1, n, u, s);
    auto q2 = pushRectVerts(geo, -s + 0.5, -s + 0.5 + N, -h2, h2, n, u, s);

    geo.pushQuad(q1[0],q2[0],q2[1],q1[1]);
    geo.pushQuad(q1[1],q2[1],q2[2],q1[2]);
    geo.pushQuad(q1[2],q2[2],q2[3],q1[3]);
    geo.pushQuad(q1[3],q2[3],q2[0],q1[0]);
    geo.pushQuad(q2[0],q2[3],q2[2],q2[1]); // inner quad for label
}

void pushSubjectBox(VRGeoData& geo, int N, float h) {
    float s = 0.5 * N + 0.5;
    pushLabelFace(geo, N,s,s,h, Vec3d(0,-1,0), Vec3d(0,0,1));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,1,0), Vec3d(0,0,1));
    pushLabelFace(geo, N,s,s,h, Vec3d(1,0,0), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(-1,0,0), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,0,1), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,0,-1), Vec3d(0,1,0));
}

void pushActionBox(VRGeoData& geo, int N, float h) {
    float s = 0.5 * N + 0.5;
    pushLabelFace(geo, N,s,s,h, Vec3d(0,-1,0), Vec3d(0,0,1));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,1,0), Vec3d(0,0,1));
    pushLabelFace(geo, N,s,s,h, Vec3d(1,0,0), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(-1,0,0), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,0,1), Vec3d(0,1,0));
    pushLabelFace(geo, N,s,s,h, Vec3d(0,0,-1), Vec3d(0,1,0));
}

void pushMsgBox(VRGeoData& geo, int N, float h) {
    float s = 0.5 * N + 0.5;
    Vec4i q1 = pushRectVerts(geo, -s, s, -h, h, Vec3d(0,-1,0), Vec3d(0,0,1), h);
    Vec4i q2 = pushRectVerts(geo, -s, s, -h, h, Vec3d(0,1,0), Vec3d(0,0,1), h);
    Vec4i q3 = pushRectVerts(geo, -s, s, -h, h, Vec3d(0,0,-1), Vec3d(0,1,0), h);
    Vec4i q4 = pushRectVerts(geo, -s, s, -h, h, Vec3d(0,0,1), Vec3d(0,1,0), h);
    Vec4i q5 = pushRectVerts(geo, -h, h, -h, h, Vec3d(1,0,0), Vec3d(0,1,0), s);
    Vec4i q6 = pushRectVerts(geo, -h, h, -h, h, Vec3d(-1,0,0), Vec3d(0,1,0), s);
    geo.pushQuad(q1[0],q1[3],q1[2],q1[1]);
    geo.pushQuad(q2[0],q2[3],q2[2],q2[1]);
    geo.pushQuad(q3[0],q3[3],q3[2],q3[1]);
    geo.pushQuad(q4[0],q4[3],q4[2],q4[1]);
    geo.pushQuad(q5[0],q5[3],q5[2],q5[1]);
    geo.pushQuad(q6[0],q6[3],q6[2],q6[1]);
}

VRGeometryPtr VRProcessLayout::newWidget(VRProcessNodePtr n, float height) {
    Color4f fg, bg;
    if (n->type == SUBJECT) { fg = Color4f(0,0,0,1); bg = Color4f(0.8,0.9,1,1); }
    if (n->type == MESSAGE) { fg = Color4f(0,0,0,1); bg = Color4f(1,1,0,1); }
    if (n->type == TRANSITION) { fg = Color4f(0,0,0,1); bg = Color4f(1,1,0,1); }
    //TODO: set different color for current actions in the process engine
    if (n->type == ACTION) { fg = Color4f(0,0,0,1); bg = Color4f(1,0.9,0.8,1); }

    int wrapN = 12;
    if (n->type == MESSAGE || n->type == TRANSITION) wrapN = 22;
    string l = n->label;
    int lineN = wrapString(l, wrapN);

    auto txt = VRText::get()->create(l, "MONO 20", 18*wrapN, 32*lineN, fg, bg);
    auto mat = VRMaterial::create("ProcessElement");
    mat->setTexture(txt, false);
    mat->setTextureParams(GL_LINEAR, GL_LINEAR);
    //mat->enableTransparency(0);
    VRGeoData geo;

    if (n->type == SUBJECT) pushSubjectBox(geo, wrapN, lineN*height*0.5);
    if (n->type == ACTION) pushActionBox(geo, wrapN, lineN*height*0.5);
    if (n->type == MESSAGE || n->type == TRANSITION) pushMsgBox(geo, wrapN, lineN*height*0.5);

    auto w = geo.asGeometry("ProcessElement");
    if (n->type == SUBJECT) w->addTag("subject");
    if (n->type == ACTION) w->addTag("action");
    if (n->type == MESSAGE) w->addTag("message");
    if (n->type == TRANSITION) w->addTag("transition");
    w->setMaterial(mat);
    w->getConstraint()->lock({1,3,4,5});
    w->getConstraint()->setReferential(ptr());
    addChild(w);
    n->widget = w;
    return w;
}

void VRProcessLayout::setProcess(VRProcessPtr p) {
    if (!p) { cout << "WARNING in ProcessLayout, setProcess: process is null!\n"; return; }

    process = p;

    auto constrainHandles = [&](VRPathtoolPtr tool) {
        for (auto h : tool->getHandles()) {
            auto c = h->getConstraint();
            c->setReferential( ptr() );
            c->lock({1,3,5});
        }
    };

    //initialize pathtool for each sbd
    for (auto subject : p->getSubjects()){
        if (!p->getBehaviorDiagram(subject->getID())) return;
        VRPathtoolPtr toolSBD = VRPathtool::create();
        toolSBD->setPersistency(0);
        toolSBDs[subject->getID()] = toolSBD;
        addChild(toolSBD);
        toolSBD->setGraph(p->getBehaviorDiagram(subject->getID()));
        constrainHandles(toolSBD);
    }
    toolSID->setGraph( p->getInteractionDiagram() );
    constrainHandles(toolSID);

    rebuild();
}

void VRProcessLayout::setEngine(VRProcessEnginePtr e) { engine = e; }

void VRProcessLayout::rebuild() {
    if (!process) return;
    clearChildren();
    addChild(toolSID);

    for(auto tool : toolSBDs) addChild(tool.second);

    auto sid = process->getInteractionDiagram();
    if (!sid) return;

    for (auto subject : process->getSubjects()){
        auto sbd = process->getBehaviorDiagram(subject->getID());
        if (!sbd) return;
    }

    buildSID();
    buildSBDs();
    update();
}

void VRProcessLayout::appendToHandle(Vec3d pos, VRProcessNodePtr node, VRPathtoolPtr ptool) {
    PosePtr pose = Pose::create(pos,Vec3d(0,0,-1),Vec3d(0,1,0));
    auto h = ptool->getHandle(node->getID());
    ptool->setHandlePose(node->getID(), pose);
    h->addChild( addElement(node) );
    h->getConstraint()->lock({1,3,5});
    h->getConstraint()->setReferential(ptr());
    h->getConstraint()->setActive(true);
}

void VRProcessLayout::setupLabel(VRProcessNodePtr message, VRPathtoolPtr ptool, vector<VRProcessNodePtr> nodes) {
    if (nodes.size() < 2) { cout << "VRProcessLayout::setupLabel, at least two nodes needed! message: " << message->getName() << endl; return; }
    if (!nodes[0] || !nodes[1]) { cout << "VRProcessLayout::setupLabel, a node is invalid! message: " << message->getName() << endl; return; }
    auto messageElement = addElement(message);

    auto id0 = nodes[0]->getID();
    auto id1 = nodes[1]->getID();

    Vec3d p;
    auto h0 = ptool->getHandle(id0);
    auto h1 = ptool->getHandle(id1);
    if (h0 && h1) p = (h0->getWorldPosition() + h1->getWorldPosition())*0.5;

    int idm = message->getID();
    ptool->setHandlePose(idm, Pose::create(p,Vec3d(0,0,-1),Vec3d(0,1,0) ));
    auto h = ptool->getHandle(idm);
    h->addChild( messageElement );
}

void VRProcessLayout::buildSID() {
    auto subjects = process->getSubjects();
	for (int i=0; i < subjects.size(); i++) {
        appendToHandle(Vec3d(0,0,i*25), subjects[i], toolSID);
	}

	for (auto message : process->getMessages()) {
        auto subjects = process->getMessageSubjects( message->getID() );
        setupLabel(message, toolSID, subjects);
	}
}

void VRProcessLayout::buildSBDs() {
    auto subjects = process->getSubjects();
	for (int i=0; i < subjects.size(); i++) {
        int sID = subjects[i]->getID();
        auto toolSBD = toolSBDs[sID];
        auto actions = process->getSubjectActions(sID);

        for (int j=0; j < actions.size(); j++) {
            appendToHandle(Vec3d((j+1)*25,0,i*25), actions[j], toolSBD);
        }

        for (auto transition : process->getTransitions(sID)) {
            auto actions = process->getTransitionActions(sID, transition->getID());
            setupLabel(transition, toolSBD, actions);
        }
	}
}

void VRProcessLayout::printHandlePositions(){
    for (auto subject : process->getSubjects()){
        cout << "subject: " << subject->getID() << endl;
        auto toolSBD = toolSBDs[subject->getID()];
        auto behavior = process->getBehaviorDiagram(subject->getID());
        for (auto node : behavior->processnodes){
            auto nid = node.second->getID();
            auto handle = toolSBD->getHandle(nid);
            auto position = handle->getWorldPosition();
            auto p = handle->getRelativePose(handle->getParent());
            cout << "node type " << node.second->type << " handle position: " << position << " parent: " << handle->getParent()->getName() << " rel pose: " << p->toString() << endl;
            handle->setRelativeDir(p->dir(), handle->getParent());
            cout << "node type " << node.second->type << " handle position: " << position << " parent: " << handle->getParent()->getName() << " rel pose: " << p->toString() << endl;
        }
    }
}

VRObjectPtr VRProcessLayout::getElement(int i) { return elements.count(i) ? elements[i].lock() : 0; }

VRObjectPtr VRProcessLayout::addElement(VRProcessNodePtr n) {
    if (!n) return 0;
    auto e = newWidget(n, height);
    elements[n->ID] = e;
    elementIDs[e.get()] = n->ID;
    return e;
}

void VRProcessLayout::remElement(VRObjectPtr o) {
    auto key = o.get();
    if (elementIDs.count(key)) {
        int nID = elementIDs[key];
        elements.erase(nID);
        elementIDs.erase(key);
        o->destroy();
        if (auto n = getProcessNode(nID)) process->remNode(n);
    }
}

int VRProcessLayout::getElementID(VRObjectPtr o) { return elementIDs.count(o.get()) ? elementIDs[o.get()] : -1; }

VRProcessNodePtr VRProcessLayout::getProcessNode(int i) { return process ? process->getNode(i) : 0; }

void VRProcessLayout::selectElement(VRGeometryPtr geo) { // TODO
    ;
}

void VRProcessLayout::setElementName(int ID, string name) {
    if (!process) return;
    auto e = dynamic_pointer_cast<VRGeometry>( getElement(ID) );
    if (!e) return;
    auto n = process->getNode(ID);
    n->label = name;

    Color4f fg, bg;
    if (n->type == SUBJECT) { fg = Color4f(0,0,0,1); bg = Color4f(0.8,0.9,1,1); }
    if (n->type == MESSAGE) { fg = Color4f(0,0,0,1); bg = Color4f(1,1,0,1); }

    int wrapN = 12;
    if (n->type == MESSAGE) wrapN = 22;
    int lineN = wrapString(name, wrapN);

    auto txt = VRText::get()->create(name, "MONO 20", 18*wrapN, 32*lineN, fg, bg);
    auto mat = VRMaterial::create("ProcessElement");
    mat->setTexture(txt, false);
    mat->setTextureParams(GL_LINEAR, GL_LINEAR);

    e->setMaterial(mat);
}

void VRProcessLayout::update(){
    toolSID->update();
	for(auto toolSBD : toolSBDs) toolSBD.second->update();
}

void VRProcessLayout::storeLayout() {
    string path = ".process_layout.plt";

    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("ProjectsList", "", "VRP"); // name, ns_uri, ns_prefix

    auto storeHandles = [&](VRPathtoolPtr tool) {
        for (auto handle : tool->getHandles()) {
            handle->setPersistency(1);
            handle->saveUnder(root);
        }
    };

    storeHandles(toolSID);
    for (auto tool : toolSBDs) storeHandles(tool.second);

    doc.write_to_file_formatted(path);
}

void VRProcessLayout::loadLayout() {
    string path = ".process_layout.plt";
    auto context = VRStorageContext::create(true);

    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());
    xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(parser.get_document()->get_root_node());

    int i = 0;
    auto loadHandles = [&](VRPathtoolPtr tool) {
        for (auto handle : tool->getHandles()) {
            auto element = handle->loadChildIFrom(root, i, context);
            i++;
        }
    };
    loadHandles(toolSID);
    for (auto tool : toolSBDs) loadHandles(tool.second);

    update();
}



