#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {}
VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) { return VRProcessLayoutPtr(new VRProcessLayout(name) ); }

VRTransformPtr VRProcessLayout::newWidget(string label, float height) {
    auto w = VRGeometry::create("ProcessElement");
    w->setPrimitive("Box", toString(label.size())+" "+toString(height)+" 1 1 1 1");
    auto txt = VRText::get()->create(label, "SANS 20", 20, Color4f(0,0,0.5,1), Color4f(1,1,1,1));
    auto mat = VRMaterial::create("ProcessElement");
    mat->setTexture(txt);
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
        e.widget = newWidget(e.label, height);

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
