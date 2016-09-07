#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {}
VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) { return VRProcessLayoutPtr(new VRProcessLayout(name) ); }

VRTransformPtr VRProcessLayout::newWidget(string label, float height) {
    auto w = VRSprite::create("Subject");
    w->setMaterial(VRMaterial::create("Subject"));
    w->setFontColor(Color4f(0,0,0.5,1));
    w->setBackColor(Color4f(1,1,1,1));
    w->setLabel(label);
    w->setSize(label.size(), height);
    w->setPickable(1);
    w->getMaterial()->setLit(0);
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
        e.widget = newWidget(e.label, height);

        Vec3f p = Vec3f(f+2,0,0);
        f += e.label.size();
        e.widget->setFrom(p);
        auto& n = diag->getNode(i);
        n.pos = p;
        Vec3f size(e.label.size(), height, height);
        n.box.update(size*0.5);
        n.box.update(-size*0.5);
    }
}
