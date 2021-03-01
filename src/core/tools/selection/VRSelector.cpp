#include "VRSelector.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

template<> string typeName(const VRSelector::VISUAL& o) { return "VRSelector::VISUAL"; }

template<> int toValue(stringstream& ss, VRSelector::VISUAL& e) {
    string s = ss.str();
    if (s == "OUTLINE") { e = VRSelector::OUTLINE; return true; }
    if (s == "OVERLAY") { e = VRSelector::OVERLAY; return true; }
    return false;
}

VRSelector::VRSelector() {
    selection = VRSelection::create();
}

VRSelectorPtr VRSelector::create() { return VRSelectorPtr( new VRSelector() ); }

VRSelector::MatStore::MatStore(VRGeometryPtr geo) {
    this->geo = geo;
    mat = geo->getMaterial();
}

void VRSelector::update() {
    deselect();
    if (!selection) return;

    // highlight selected objects
    for (auto g : selection->getSelected()) {
        auto geo = g.lock();
        if (!geo) continue;
        if (!geo->getMaterial()) continue;
        orig_mats.push_back(MatStore(geo));

        VRMaterialPtr mat = getMat();
        mat->prependPasses(geo->getMaterial());
        if (visual == OUTLINE) {
            mat->setActivePass(0);
            mat->setStencilBuffer(true, 1,-1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_REPLACE);
        }
        geo->setMaterial(mat);
    }

    // visualise subselections
    if (selection->getSubselections().size() <= 1) return; // hack! should be zero!
    subselection = VRGeometry::create("subsel");
    subselection->setPersistency(0);
    for (auto m : selection->getSubselections()) {
        if (!m.first) continue;
        auto s = m.first->copySelection(selection);
        s->setWorldMatrix(m.first->getWorldMatrix());
        subselection->merge(s);
    }

    if (!subselection->getMesh()->geo) return;
    if (!subselection->getMesh()->geo->getPositions()) return;
    int N = subselection->getMesh()->geo->getPositions()->size();

    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
    lengths->addValue(N);
    for (int i=0; i<N; i++) inds->addValue(i);

    subselection->setType(GL_POINTS);
    subselection->setLengths(lengths);
    subselection->setIndices(inds);

    auto m = VRMaterial::create("sel");
    m->setLit(false);
    m->setDiffuse(color);
#ifndef OSG_OGL_ES2
    m->setFrontBackModes(GL_LINE, GL_LINE);
#endif
    m->setPointSize(width);
    subselection->setMaterial(m);

    auto scene = VRScene::getCurrent();
    if (!scene) return;
    scene->getRoot()->addChild(subselection);
}

VRMaterialPtr VRSelector::getMat() {
    VRMaterialPtr mat = VRMaterial::create("VRSelector");

    // stencil buffer
    /*mat->setFrontBackModes(GL_POINT, GL_POINT);
    mat->setDiffuse(color);
    mat->setPointSize(8);
    mat->setLit(false);
    mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    mat->addPass();*/

    mat->setDiffuse(color);
    mat->setLineWidth(width, smooth);
    if (visual == OUTLINE) {
#ifndef OSG_OGL_ES2
        mat->setFrontBackModes(GL_LINE, GL_LINE);
#endif
        mat->setLit(false);
        mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);
        mat->ignoreMeshColors(true);
    }
    if (visual == OVERLAY) {
        mat->setTransparency(transparency);
        mat->ignoreMeshColors(true);
        //mat->addPass();
    }

    return mat;
}

void VRSelector::deselect() {
    for (auto ms : orig_mats) if ( auto geo = ms.geo.lock() ) geo->setMaterial(ms.mat);
    orig_mats.clear();
    if (subselection) subselection->destroy();
    subselection.reset();
}

void VRSelector::clear() { selection->clear(); selected = 0; update(); }

void VRSelector::select(VRObjectPtr obj, bool append, bool recursive) {
    if (!append) {
        clear();
        selection->apply(obj, true, recursive);
    } else {
        auto s = VRSelection::create();
        s->apply(obj, true, recursive);
        selection->append(s);
    }
    selected = obj;
    update();
}

VRObjectPtr VRSelector::getSelected() {
    if (selected) return selected;
    auto objs = selection->getSelected();
    if (objs.size() > 0) return objs[0].lock();
    return 0;
}

void VRSelector::set(VRSelectionPtr s) {
    if (s != selection) {
        clear();
        selection = s;
    }
    selected = 0;
    update();
}

void VRSelector::add(VRSelectionPtr s) {
    if (!selection) selection = s;
    else selection->append(s);
    selected = 0;
    update();
}

void VRSelector::setBorder(int width, bool smooth) {
    this->width = width;
    this->smooth = smooth;
    update();
}

void VRSelector::setVisual(VISUAL v) { visual = v; update(); };
void VRSelector::setColor(Color3f c, float t) { color = c; transparency = t; update(); }
VRSelectionPtr VRSelector::getSelection() { return selection; }

