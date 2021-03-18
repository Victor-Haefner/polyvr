#include "VRStroke.h"
#include "VRGeoData.h"
#include "core/math/path.h"
#include "core/math/polygon.h"
#include "core/objects/material/VRMaterial.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
#include "core/utils/toString.h"

#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTriangleIterator.h>

template<> string typeName(const OSG::VRStroke::CAP& s) { return "StrokeCap"; }

template<> int toValue(stringstream& ss, OSG::VRStroke::CAP& c) {
    string s; ss >> s;
    c = OSG::VRStroke::NONE;
    if (s == "ROUND") c = OSG::VRStroke::ROUND;
    if (s == "ARROW") c = OSG::VRStroke::ARROW;
    return s.length();
}

OSG_BEGIN_NAMESPACE;
using namespace std;

VRStroke::VRStroke(string name) : VRGeometry(name) { type = "Stroke"; }
VRStrokePtr VRStroke::create(string name) { return shared_ptr<VRStroke>(new VRStroke(name) ); }
VRStrokePtr VRStroke::ptr() { return static_pointer_cast<VRStroke>( shared_from_this() ); }

void VRStroke::clear() {
    paths.clear();
    polygons.clear();
    VRGeometry::clear();
}

void VRStroke::addPath(PathPtr p) { paths.push_back(p); }
void VRStroke::setPath(PathPtr p) { paths.clear(); addPath(p); }
void VRStroke::setPaths(vector<PathPtr> p) { paths = p; }
vector<PathPtr> VRStroke::getPaths() { return paths; }

void VRStroke::addPolygon(VRPolygonPtr p) { polygons.push_back(p); }

void VRStroke::strokeProfile(vector<Vec3d> profile, bool closed, bool lit, bool doColor, CAP l, CAP r) {
    mode = 0;
    this->profile = profile;
    this->closed = closed;
    this->lit = lit;
    this->doColor = doColor;
    cap_beg = l;
    cap_end = r;

    Vec3d pCenter;
    for (auto p : profile) pCenter += p;
    pCenter *= 1.0/profile.size();

    VRGeoData data;
    bool doCaps = closed && profile.size() > 1;
    Vec3d z = Vec3d(0,0,1);

    auto paths = this->paths;
    for (auto p : polygons) paths.push_back( p->toPath() );

    float Lp = 0;
    vector<Vec2d> tcs(1);
    for (unsigned int i=1; i<profile.size(); i++) {
        Lp += (profile[i]-profile[i-1]).length();
        tcs.push_back( Vec2d(0,Lp) );
    }
    for (unsigned int i=1; i<profile.size(); i++) tcs[i] /= Lp;

    clearChildren();
    for (auto path : paths) {
        auto pnts = path->getPositions();
        auto directions = path->getDirections();
        auto up_vectors = path->getUpVectors();
        auto cols = path->getColors();


        //float L = path->getLength();
        float l = 0;
        Vec3d _p;
        for (unsigned int j=0; j<pnts.size(); j++) {
            Vec3d p = pnts[j];
            Vec3d n = directions[j];
            Vec3d u = up_vectors[j];
            Color3f c = Vec3f(cols[j]);
            if (j > 0) l += (p-pnts[j-1]).length();

            Matrix4d m;
            MatrixLookAt(m, Vec3d(0,0,0), n, u);

            bool begArrow1 = (j == 0) && (cap_beg == ARROW);
            bool begArrow2 = (j == 1) && (cap_beg == ARROW);
            bool endArrow1 = (j == pnts.size()-2) && (cap_end == ARROW);
            bool endArrow2 = (j == pnts.size()-1) && (cap_end == ARROW);

            // add new profile points and normals
            for (unsigned int i=0; i<profile.size(); i++) {
                Vec3d pos = profile[i];
                Vec2d tc = tcs[i];
                tc[0] = l;
                if (endArrow1 || begArrow2) pos += (pos-pCenter)*2.5;
                if (endArrow2 || begArrow1) pos = pCenter;
                m.mult(pos, pos);

                Vec3d norm = pos; norm.normalize();
                if (!doColor) data.pushVert(p+pos, norm, tc);
                else data.pushVert(p+pos, norm, c, tc);
            }

            if (j==0) continue;

            if (profile.size() == 1) data.pushLine();
            else { // add quad
                for (unsigned int k=0; k<profile.size()-1; k++) {
                    int N1 = data.size() - 2*profile.size() + k;
                    int N2 = data.size() -   profile.size() + k;
                    data.pushQuad(N1, N2, N2+1, N1+1);
                }

                if (closed) {
                    int N0 = data.size() - 2*profile.size();
                    int N1 = data.size() - profile.size() - 1;
                    int N2 = data.size() - 1;
                    data.pushQuad(N1, N2, N1+1, N0);
                }
            }
        }
    }

    // caps
    if (doCaps) {
        bool begRound = (cap_beg == ROUND);
        bool endRound = (cap_beg == ROUND);

        for (unsigned int i=0; i<paths.size(); i++) {
            if (paths[i]->isClosed()) continue;

            vector<Vec3d> pnts = paths[i]->getPositions();
            vector<Vec3d> directions = paths[i]->getDirections();
            vector<Vec3d> up_vectors = paths[i]->getUpVectors();
            vector<Vec3d> cols = paths[i]->getColors();

            if (pnts.size() == 0) { cout << "VRStroke::strokeProfile path size 0!\n"; continue; }

            Matrix4d m;

             // first cap
            Vec3d p = pnts[0];
            Vec3d pc = p;
            Vec3d n = directions[0];
            Vec3d u = up_vectors[0];
            Color3f c = Vec3f(cols[0]);
            MatrixLookAt(m, Vec3d(0,0,0), n, u);
            Vec3d tmp; m.mult(pCenter, tmp);

            if (begRound) {
                Vec3d nc = n; nc.normalize();
                pc -= nc*profile[0].length();
            }

            int Ni = data.size();
            if (!doColor) data.pushVert(pc + tmp, -n);
            else data.pushVert(pc + tmp, -n, Vec3f(c));

            for (unsigned int k=0; k<profile.size(); k++) {
                Vec3d tmp = profile[k];
                m.mult(tmp, tmp);

                if (begRound) { n = -tmp; n.normalize(); }

                if (!doColor) data.pushVert(p + tmp, -n);
                else data.pushVert(p + tmp, -n, Vec3f(c));
            }

            for (unsigned int k=1; k<=profile.size(); k++) {
                int j = k+1;
                if (k == profile.size()) j = 1;
                data.pushTri(Ni, Ni+k, Ni+j);
            }

             // last cap
            int N = pnts.size()-1;
            Ni = data.size();
            p = pnts[N];
            pc = p;
            n = directions[N];
            u = up_vectors[N];
            c = Vec3f(cols[N]);
            MatrixLookAt(m, Vec3d(0,0,0), n, u);
            m.mult(pCenter, tmp);

            if (endRound) {
                Vec3d nc = n; nc.normalize();
                pc += nc*profile[0].length();
            }

            if (!doColor) data.pushVert(pc + tmp, n);
            else data.pushVert(pc + tmp, n, c);

            for (unsigned int k=0; k<profile.size(); k++) {
                Vec3d tmp = profile[k];
                m.mult(tmp, tmp);

                if (endRound) { n = tmp; n.normalize(); }

                if (!doColor) data.pushVert(p + tmp, n);
                else data.pushVert(p + tmp, n, c);
            }

            for (unsigned int k=1; k<=profile.size(); k++) {
                int j = k+1;
                if (k == profile.size()) j = 1;
                data.pushTri(Ni, Ni+j, Ni+k);
            }
        }
    }

    data.apply( ptr() );
    if (auto m = getMaterial()) m->setLit(lit);
}

void VRStroke::strokeStrew(VRGeometryPtr geo) {
    if (geo == 0) return;

    mode = 1;
    strewGeo = geo;

    clearChildren();
    for (unsigned int i=0; i<paths.size(); i++) {
        vector<Vec3d> pnts = paths[i]->getPositions();
        for (unsigned int j=0; j<pnts.size(); j++) {
            Vec3d p = pnts[j];
            VRGeometryPtr g = static_pointer_cast<VRGeometry>(geo->duplicate());
            addChild(g);
            g->translate(p);
        }
    }
}

void VRStroke::setDoColor(bool b) { doColor = b; }

void VRStroke::update() {
    switch (mode) {
        case 0:
            strokeProfile(profile, closed, lit, doColor, cap_beg, cap_end);
            break;
        case 1:
            strokeStrew(strewGeo);
            break;
        default:
            break;
    }
}

vector<Vec3d> VRStroke::getProfile() { return profile; }
PathPtr VRStroke::getPath() { return paths[0]; }

void VRStroke::convertToRope() {
#ifndef WITHOUT_BULLET
    getPhysics()->setDynamic(true);
    getPhysics()->setShape("Rope");
    getPhysics()->setSoft(true);
    getPhysics()->setPhysicalized(true);
#endif
}

OSG_END_NAMESPACE;
