#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {}
VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) { return VRProcessLayoutPtr(new VRProcessLayout(name) ); }


/* IDEAS

- one material for all widgets
    - one big texture for all labels


*/


VRTransformPtr VRProcessLayout::newWidget(VRProcess::Node& n, float height) {
    auto txt = VRText::get()->create(n.label, "SANS 16", 20, Color4f(0,0,0.5,1), Color4f(1,1,1,1));
    auto mat = VRMaterial::create("ProcessElement");
    mat->setTexture(txt);
    VRGeoData geo;

    if (n.type == VRProcess::SUBJECT) {
        float s = 0.5 * n.label.size() + 0.5;
        float h = height*0.5;
        float w1 = -s + 0.5;
        float w2 = w1 + n.label.size();

        int q1 = geo.pushVert(Pnt3f(-s,-s,1), Vec3f(0,0,1), Vec2f(0,0));
        int q2 = geo.pushVert(Pnt3f(s,-s,1), Vec3f(0,0,1), Vec2f(1,0));
        int q3 = geo.pushVert(Pnt3f(s,s,1), Vec3f(0,0,1), Vec2f(1,1));
        int q4 = geo.pushVert(Pnt3f(-s,s,1), Vec3f(0,0,1), Vec2f(0,1));
        int v1 = geo.pushVert(Pnt3f(w1,-h,1), Vec3f(0,0,1), Vec2f(0,0));
        int v2 = geo.pushVert(Pnt3f(w2,-h,1), Vec3f(0,0,1), Vec2f(1,0));
        int v3 = geo.pushVert(Pnt3f(w2,h,1), Vec3f(0,0,1), Vec2f(1,1));
        int v4 = geo.pushVert(Pnt3f(w1,h,1), Vec3f(0,0,1), Vec2f(0,1));
        geo.pushQuad(q1,v1,v2,q2);
        geo.pushQuad(q2,v2,v3,q3);
        geo.pushQuad(q3,v3,v4,q4);
        geo.pushQuad(q4,v4,v1,q1);
        geo.pushQuad(v1,v2,v3,v4); // inner quad for label
    }

    if (n.type == VRProcess::MESSAGE) {
        float s = 0.5 * n.label.size() + 0.5;
        float h = height*0.5;

        int v1 = geo.pushVert(Pnt3f(-s,-h,1), Vec3f(0,0,1), Vec2f(0,0));
        int v2 = geo.pushVert(Pnt3f(s,-h,1), Vec3f(0,0,1), Vec2f(1,0));
        int v3 = geo.pushVert(Pnt3f(s,h,1), Vec3f(0,0,1), Vec2f(1,1));
        int v4 = geo.pushVert(Pnt3f(-s,h,1), Vec3f(0,0,1), Vec2f(0,1));
        geo.pushQuad(v1,v2,v3,v4);
    }

    auto w = geo.asGeometry("ProcessElement");
    mat->setLit(0);
    w->setMaterial(mat);
    w->setPickable(1);
    w->getConstraint()->setTConstraint(Vec3f(0,0,1), VRConstraint::PLANE);
    w->getConstraint()->setRConstraint(Vec3f(0,1,0), VRConstraint::POINT);
    addChild(w);
    return w;
}

void VRProcessLayout::setProcess(VRProcessPtr p) {
    process = p;

    clearChildren();

    float height = 2;
    float f=0;
    auto diag = process->getInteractionDiagram();
    for (int i=0; i<diag->getElements().size(); i++) {
        auto& e = diag->getElement(i);
        e.widget = newWidget(e, height);

        Vec3f p = Vec3f(f, 0.01*(rand()%100), 0);
        f += e.label.size()+2;
        e.widget->setFrom(p);

        auto& n = diag->getNode(i);
        n.pos = p;
        Vec3f size(e.label.size(), height, height);
        n.box.update(size*0.5);
        n.box.update(-size*0.5);
    }
}
