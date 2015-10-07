#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeometry.h>

using namespace OSG;

VRSelection::VRSelection() { clear(); }

shared_ptr<VRSelection> VRSelection::create() { return shared_ptr<VRSelection>( new VRSelection() ); }

bool VRSelection::vertSelected(Vec3f p) { return false; }
bool VRSelection::objSelected(VRGeometryPtr geo) { return false; }
bool VRSelection::partialSelected(VRGeometryPtr geo) { return false; }

void VRSelection::add(VRGeometryPtr geo, vector<int> subselection) {
    auto k = geo.get();
    if (selected.count(k) == 0) selected[k] = selection_atom();
    selected[k].geo = geo;
    selected[k].subselection = subselection;
}

void VRSelection::clear() {
    selected.clear();
    bbox.clear();
}

void VRSelection::apply(VRObjectPtr tree) {
    if (!tree) return;
    auto geos = tree->getChildren(true, "Geometry");
    if (tree->getType() == "Geometry") geos.push_back(tree);
    for (auto g : geos) {
        VRGeometryPtr geo = static_pointer_cast<VRGeometry>(g);

        selection_atom a;
        a.geo = geo;
        if ( objSelected(geo) );
        else if ( partialSelected(geo) ) a.partial = true;
        else continue;
        selected[geo.get()] = a;
    }
}

vector<VRGeometryWeakPtr> VRSelection::getPartials() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) if (g.second.partial) res.push_back(g.second.geo);
    return res;
}

vector<VRGeometryWeakPtr> VRSelection::getSelected() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) if (!g.second.partial) res.push_back(g.second.geo);
    return res;
}

void VRSelection::updateSubselection() {
    for (auto s : selected) updateSubselection(s.second.geo.lock());
}

void VRSelection::updateSubselection(VRGeometryPtr geo) {
    if (!geo) return;
    if ( !selected.count( geo.get() ) ) {
        selection_atom s;
        s.geo = geo;
        selected[geo.get()] = s;
    }

    auto& sel = selected[geo.get()];
    Matrix m = geo->getWorldMatrix();
    sel.subselection.clear();
    auto pos = geo->getMesh()->getPositions();
    for (int i=0; i<pos->size(); i++) {
        Pnt3f p = pos->getValue<Pnt3f>(i);
        m.mult(p,p);
        if (vertSelected(Vec3f(p))) {
            bbox.update(Vec3f(p));
            sel.subselection.push_back(i);
        }
    }
}

vector<int> VRSelection::getSubselection(VRGeometryPtr geo) {
    if ( !selected.count( geo.get() ) ) updateSubselection(geo);
    if ( !selected.count( geo.get() ) ) return vector<int>();
    return selected[geo.get()].subselection;
}
