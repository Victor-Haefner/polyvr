#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeometry.h>

using namespace OSG;

VRSelection::VRSelection() {}

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
}

void VRSelection::apply(VRObjectPtr tree) {
    auto geos = tree->getChildren(true, "Geometry");
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

vector<int> VRSelection::getSubselection(VRGeometryPtr geo) {
    if ( !selected.count( geo.get() ) ) {
        Matrix m = geo->getWorldMatrix();
        vector<int> res;
        auto pos = geo->getMesh()->getPositions();
        for (int i=0; i<pos->size(); i++) {
            Pnt3f p = pos->getValue<Pnt3f>(i);
            m.mult(p,p);
            if (vertSelected(Vec3f(p))) res.push_back(i);
        }
        selected[geo.get()].subselection = res;
    }

    return selected[geo.get()].subselection;
}
