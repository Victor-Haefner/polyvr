#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"
#include "core/tools/VRPathtool.h"

#include <OpenSG/OSGMatrixUtility.h>

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {}
VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) {
    auto l = VRProcessLayoutPtr(new VRProcessLayout(name) );
    l->init();
    return l;
}

void VRProcessLayout::init() {
    toolSID = VRPathtool::create();
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
    w->getConstraint()->setTConstraint(Vec3d(0,1,0), VRConstraint::PLANE);
    w->getConstraint()->setReferential(ptr());
    w->getConstraint()->setActive(true);
    addChild(w);
    n->widget = w;
    return w;
}

void VRProcessLayout::setProcess(VRProcessPtr p) {
    process = p;

    //initialize pathtool for each sbd
    for (auto subject : process->getSubjects()){
        if (!process->getBehaviorDiagram(subject->getID())) return;
        VRPathtoolPtr toolSBD = VRPathtool::create();
        toolSBDs[subject->getID()] = toolSBD;
        addChild(toolSBD);
    }

    rebuild();

}

void VRProcessLayout::setEngine(VRProcessEnginePtr e) { engine = e; }

void VRProcessLayout::rebuild() {
    if (!process) return;
    clearChildren();
    addChild(toolSID);
    for(tool : toolSBDs){
        addChild(tool.second);
    }
    auto sid = process->getInteractionDiagram();
    if (!sid) return;

    buildSID();
    buildSBDs();

    for (auto subject : process->getSubjects()){
        auto sbd = process->getBehaviorDiagram(subject->getID());
        if (!sbd) return;
    }

	toolSID->update();
	for(tool : toolSBDs){
        tool.second->update();
	}
}

void VRProcessLayout::buildSID(){
    int i = 0;
	for (auto subject : process->getSubjects()) {
        PosePtr pose = Pose::create(Vec3d(0,0,i*25),Vec3d(0,0,-1),Vec3d(0,1,0));
		auto n = toolSID->addNode(pose);
		auto h = toolSID->getHandle(n);
		h->addChild(addElement(subject) );
		i++;
	}

	for (auto message : process->getMessages()) {
		auto messageElement = addElement(message);
		auto subjects = process->getMessageSubjects( message->getID() );


		auto id0 = subjects[0]->getID();
		auto id1 = subjects[1]->getID();

		auto h0 = toolSID->getHandle(id0);
		auto h1 = toolSID->getHandle(id1);
		auto p = (h0->getWorldPosition() + h1->getWorldPosition())*0.5;
		auto n = toolSID->addNode( Pose::create(p,Vec3d(0,0,-1),Vec3d(0,1,0) ) );
		auto h = toolSID->getHandle(n);
		h->addChild( messageElement );

		Vec3d norm = Vec3d(1,0,0);
		int idm = message->getID();
		toolSID->connect(id0, idm, false, true, norm, norm);
		toolSID->connect(idm, id1, false, true, norm, norm);
	}
}

void VRProcessLayout::buildSBDs(){
    int i = 0;
	for (auto subject : process->getSubjects()) {
        int j = 1;
        auto toolSBD = toolSBDs[subject->getID()];
        for (auto action : process->getSubjectActions(subject->getID())){
            PosePtr pose = Pose::create(Vec3d(j*25,0,i*25),Vec3d(0,0,-1),Vec3d(-1,0,0));
            auto n = toolSBD->addNode(pose);
            auto h = toolSBD->getHandle(n);
            h->addChild(addElement(action) );
            j++;
        }

        for (auto transition : process->getTransitions(subject->getID())){
            auto transitionElement = addElement(transition);
            auto actions = process->getTransitionActions(subject->getID(), transition->getID());

            auto id0 = actions[0]->getID();
            auto id1 = actions[1]->getID();

            auto h0 = toolSBD->getHandle(id0);
            auto h1 = toolSBD->getHandle(id1);
            auto p = (h0->getWorldPosition() + h1->getWorldPosition())*0.5;
            auto n = toolSBD->addNode(Pose::create(p,Vec3d(0,0,-1), Vec3d(0,1,0)));
            auto h = toolSBD->getHandle(n);
            h->addChild(transitionElement);

            Vec3d norm = Vec3d(1,0,0);
            int idt = transition->getID();
            toolSBD->connect(id0, idt, false, true, norm, norm);
            toolSBD->connect(idt, id1, false, true, norm, norm);
        }
        i++;
	}
}

/*void VRProcessLayout::rebuild() {
    if (!process) return;
    clearChildren();
    float f=0;
    auto diag = process->getInteractionDiagram();
    if (!diag) return;
    for (int i=0; i<diag->size(); i++) {
        auto& e = diag->processnodes[i];
        auto geo = newWidget(e, height);

        Vec3d p = Vec3d(f, 0, 0.01*(rand()%100));
        f += e->label.size()+2;
        e->widget->setFrom(p);

        auto& n = diag->getNode(i);
        n.box.updateFromGeometry(geo);
        n.box.setCenter(p);
    }
}*/

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




