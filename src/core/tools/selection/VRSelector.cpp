#include "VRSelector.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

//#include <OpenSG/OSGMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSelector::VRSelector() {
    color = Vec3f(0.2, 0.65, 0.9);
    selection = VRSelection::create();
}

void VRSelector::update() {
    if (!selection) return;
    deselect();

    // highlight selected objects
    for (auto g : selection->getSelected()) {
        auto geo = g.lock();
        if (!geo) continue;

        auto omat = geo->getMaterial();
        orig_mats[geo.get()] = omat;

        VRMaterialPtr mat = getMat();
        //mat->appendPasses(omat);
        mat->prependPasses(omat);
        mat->setActivePass(0);
        mat->setStencilBuffer(true, 1,-1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_REPLACE);
        geo->setMaterial(mat);
    }

    // visualise subselections
    if (subselection) subselection->destroy();
    subselection = VRGeometry::create("subsel");
    for (auto m : selection->getSubselections()) {
        if (!m.first) continue;
        auto s = m.first->copySelection(selection);
        s->setWorldMatrix(m.first->getWorldMatrix());
        subselection->merge(s);
    }

    auto m = VRMaterial::create("sel");
    m->setLit(false);
    m->setDiffuse(color);
    m->setFrontBackModes(GL_LINE, GL_LINE);
    subselection->setMaterial(m);

    auto scene = VRSceneManager::getCurrent();
    if (!scene) return;
    scene->getRoot()->addChild(subselection);
}

VRMaterialPtr VRSelector::getMat() {
    VRMaterialPtr mat = VRMaterial::create("VRSelector");

    // stencil buffer
    mat->setFrontBackModes(GL_POINT, GL_POINT);
    mat->setDiffuse(color);
    mat->setPointSize(8);
    mat->setLit(false);
    mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    mat->addPass();

    mat->setFrontBackModes(GL_LINE, GL_LINE);
    mat->setDiffuse(color);
    mat->setLineWidth(8);
    mat->setLit(false);
    mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    //mat->addPass();
    //mat->setDiffuse(color);
    //mat->setLit(false);

    return mat;
}

void VRSelector::deselect() {
    if (!selection) return;

    for (auto g : selection->getSelected()) {
        auto geo = g.lock();
        if (!geo) continue;
        if (orig_mats.count(geo.get()) == 0) continue;
        geo->setMaterial(orig_mats[geo.get()]);
    }

    orig_mats.clear();

    if (subselection) subselection->destroy();
    subselection.reset();
}

void VRSelector::clear() {
    selection->clear();
    update();
}

void VRSelector::select(VRObjectPtr obj) {
    clear();
    selection->apply(obj, true);
    update();
}

void VRSelector::select(VRSelectionPtr s) {
    clear();
    selection = s;
    update();
}

void VRSelector::setColor(Vec3f c) { color = c; }
VRSelectionPtr VRSelector::getSelection() { return selection; }

OSG_END_NAMESPACE;
