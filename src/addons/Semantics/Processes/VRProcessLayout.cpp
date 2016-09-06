#include "VRProcessLayout.h"
#include "VRProcess.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

VRProcessLayout::VRProcessLayout(string name) : VRTransform(name) {}
VRProcessLayout::~VRProcessLayout() {}

VRProcessLayoutPtr VRProcessLayout::ptr() { return static_pointer_cast<VRProcessLayout>( shared_from_this() ); }
VRProcessLayoutPtr VRProcessLayout::create(string name) { return VRProcessLayoutPtr(new VRProcessLayout(name) ); }

void VRProcessLayout::setProcess(VRProcessPtr p) {
    process = p;

    clearChildren();

    for (auto& n : process->getInteractionDiagram()->getElements()) {
        auto w = VRSprite::create("Subject");
        w->setMaterial(VRMaterial::create("Subject"));
        w->setLabel(n.label);
        w->setSize(n.label.size()*10, 20);
        n.widget = w;
        addChild(w);
    }
}
