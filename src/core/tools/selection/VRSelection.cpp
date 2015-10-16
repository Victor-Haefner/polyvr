#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeometry.h>

using namespace OSG;

VRSelection::VRSelection() { clear(); }

VRSelectionPtr VRSelection::create() { return VRSelectionPtr( new VRSelection() ); }

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

void VRSelection::apply(VRObjectPtr tree, bool force) {
    if (!tree) return;

    vector<VRGeometryPtr> geos;
    for ( auto c : tree->getChildren(true) ) if (c->hasAttachment("geometry")) geos.push_back( static_pointer_cast<VRGeometry>(c) );
    if ( tree->hasAttachment("geometry") ) geos.push_back( static_pointer_cast<VRGeometry>(tree) );

    for (auto geo : geos) {
        selection_atom a;
        a.geo = geo;
        if ( objSelected(geo) || force);
        else if ( partialSelected(geo) ) a.partial = true;
        else continue;
        selected[geo.get()] = a;
    }
}

void VRSelection::append(VRSelectionPtr sel) {
    ;
}

vector<VRGeometryWeakPtr> VRSelection::getPartials() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) if (g.second.partial) res.push_back(g.second.geo);
    return res;
}

vector<VRGeometryWeakPtr> VRSelection::getSelected() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) {
        if (!g.second.partial) {
            res.push_back(g.second.geo);
        }
    }
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
    if (!geo->getMesh()) return;
    auto pos = geo->getMesh()->getPositions();
    if (!pos) return;
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
    if (!geo) return vector<int>();
    if ( !selected.count( geo.get() ) ) updateSubselection(geo);
    if ( !selected.count( geo.get() ) ) return vector<int>();
    return selected[geo.get()].subselection;
}

map< VRGeometryPtr, vector<int> > VRSelection::getSubselections() {
    map< VRGeometryPtr, vector<int> > res;
    for (auto s : selected) {
        auto sp = s.second.geo.lock();
        if (sp) res[sp] = s.second.subselection;
    }
    return res;
}


