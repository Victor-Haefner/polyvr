#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/math/boundingbox.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeometry.h>

#ifndef WITHOUT_LAPACKE_BLAS
#ifdef _WIN32
#include "core/math/lapack/lapacke.h"
#else
#include <lapacke.h>
#define dgeev LAPACKE_dgeev_work
#endif
#endif

using namespace OSG;

template<> string typeName(const VRSelection& t) { return "Selection"; }

VRSelection::VRSelection() { clear(); }

VRSelectionPtr VRSelection::create() { return VRSelectionPtr( new VRSelection() ); }

bool VRSelection::vertSelected(Vec3d p) { return false; }
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
    if (bbox) bbox->clear();
    else bbox = BoundingboxPtr( new Boundingbox() );
}

void VRSelection::apply(VRObjectPtr tree, bool force, bool recursive) {
    if (!tree) return;

    vector<VRGeometryPtr> geos;
    if (recursive) for ( auto c : tree->getChildren(true) ) if (c->hasTag("geometry")) geos.push_back( static_pointer_cast<VRGeometry>(c) );
    if ( tree->hasTag("geometry") ) geos.push_back( static_pointer_cast<VRGeometry>(tree) );

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
    for (auto& s : sel->selected) {
        if (!selected.count(s.first)) selected[s.first] = s.second;
        else {
            auto& s1 = selected[s.first].subselection;
            auto& s2 = s.second.subselection;
            s1.insert( s1.end(), s2.begin(), s2.end() );
        }
    }
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
    Matrix4d m = geo->getWorldMatrix();
    sel.subselection.clear();
    if (!geo->getMesh()) return;
    auto pos = geo->getMesh()->geo->getPositions();
    if (!pos) return;
    for (unsigned int i=0; i<pos->size(); i++) {
        Pnt3d p = Pnt3d(pos->getValue<Pnt3f>(i));
        m.mult(p,p);
        if (vertSelected(Vec3d(p))) {
            if (bbox) bbox->update(Vec3d(p));
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

Vec3d VRSelection::computeCentroid() {
    Vec3d res;
    int N = 0;
    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->geo->getPositions();
        int n = s.second.subselection.size();
        if (n > 0) {
            N += n;
            for (auto i : s.second.subselection) {
                auto p = Vec3d(pos->getValue<Pnt3f>(i));
                res += p;
            }
        } else {
            N += pos->size();
            for (unsigned int i = 0; i< pos->size(); i++) {
                auto p = Vec3d(pos->getValue<Pnt3f>(i));
                res += p;
            }
        }
    }

    if (N > 0) res *= 1.0/N;
    cout << " centroid: " << res << endl;
    return res;
}

Matrix4d VRSelection::computeCovMatrix() {
    Vec3d center = computeCentroid();
    int N = 0;
    Matrix4d res(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0); // 3x3?

    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->geo->getPositions();
        int n = s.second.subselection.size();
        if (n > 0) {
            N += n;
            for (auto i : s.second.subselection) {
                auto pg = Vec3d(pos->getValue<Pnt3f>(i));
                Vec3d p = pg - center;
                res[0][0] += p[0]*p[0];
                res[1][1] += p[1]*p[1];
                res[2][2] += p[2]*p[2];
                res[0][1] += p[0]*p[1];
                res[0][2] += p[0]*p[2];
                res[1][2] += p[1]*p[2];
            }
        } else {
            N += pos->size();
            for (unsigned int i = 0; i< pos->size(); i++) {
                auto pg = Vec3d(pos->getValue<Pnt3f>(i));
                Vec3d p = pg - center;
                res[0][0] += p[0]*p[0];
                res[1][1] += p[1]*p[1];
                res[2][2] += p[2]*p[2];
                res[0][1] += p[0]*p[1];
                res[0][2] += p[0]*p[2];
                res[1][2] += p[1]*p[2];
            }
        }
    }

    cout << "VRSelection::computeCovMatrix " << center << " " << N << endl;

    for (int i=0; i<3; i++)
        for (int j=i; j<3; j++) res[i][j] *= 1.0/N;

    res[1][0] = res[0][1];
    res[2][0] = res[0][2];
    res[2][1] = res[1][2];

    res[3][0] = center[0];
    res[3][1] = center[1];
    res[3][2] = center[2];

    cout << " covariance matrix: " << res[0][0] << " " << res[1][1] << " " << res[2][2] << " " << res[0][1] << " " << res[0][2] << " " << res[1][2] << endl;
    return res;
}

#ifndef WITHOUT_LAPACKE_BLAS
Matrix4d VRSelection::computeEigenvectors(Matrix4d m) {
    int n = 3, lda = 3, ldvl = 3, ldvr = 3, info, lwork;
    double wkopt;
    double* work;
    double wr[3], wi[3], vl[9], vr[9];
    double a[9] = { m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2] };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR
#ifdef _WIN32
    //info = dgeev(LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr);
    lwork = -1;
    dgeev("Vectors", "Vectors", &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, &wkopt, &lwork, &info);
    lwork = (int)wkopt;
    work = new double[lwork];
    dgeev("Vectors", "Vectors", &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, work, &lwork, &info);
    delete work;
#else
    lwork = -1;
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr, &wkopt, lwork);
    lwork = (int)wkopt;
    work = new double[lwork];
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr,  work, lwork);
    delete work;
#endif

    if ( info > 0 ) { cout << "Warning: computeEigenvalues failed!\n"; return Matrix4d(); } // Check for convergence

    Vec3i o(0,1,2);
    if (wr[0] > wr[1]) swap(o[0], o[1]);
    if (wr[1] > wr[2]) swap(o[1], o[2]);
    if (wr[0] > wr[1]) swap(o[0], o[1]);

    Matrix4d res;
    res[0] = Vec4d(vl[o[2]*3], vl[o[2]*3+1], vl[o[2]*3+2], 0);
    res[1] = Vec4d(vl[o[1]*3], vl[o[1]*3+1], vl[o[1]*3+2], 0);
    res[2] = Vec4d(vl[o[0]*3], vl[o[0]*3+1], vl[o[0]*3+2], 0);
    res[3] = Vec4d(wr[o[2]], wr[o[1]], wr[o[0]], 0);
    return res;
}

Pose VRSelection::computePCA() {
    Pose res;
    Matrix4d cov = computeCovMatrix();
    Matrix4d ev  = computeEigenvectors(cov);

    cout << "computePCA:\n" << cov << "\n\n" << ev << endl;
    res.set(Vec3d(ev[3]), Vec3d(ev[0]), Vec3d(ev[2]));
    return res;
}
#endif

void VRSelection::selectPlane(Pose p, float threshold) {
    Vec3d N = p.up();
    Plane plane( Vec3f(N), Pnt3f(p.pos()) );

    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->geo->getPositions();
        auto norms = geo->getMesh()->geo->getNormals();
        s.second.subselection.clear();
        for (unsigned int i=0; i<pos->size(); i++) {
            auto p = pos->getValue<Pnt3f>(i);
            auto n = Vec3d(norms->getValue<Vec3f>(i));
            auto d = plane.distance(p);
            auto a = n.dot(N);
            if ( abs(d) > threshold) continue;
            if ( abs(threshold*a/d) < 0.8) continue;
            s.second.subselection.push_back(i);
        }
    }
}




