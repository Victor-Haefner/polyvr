#include "VRBRepBound.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGVector.h>

using namespace OSG;

VRBRepBound::VRBRepBound() {}
VRBRepBound::~VRBRepBound() {}

VRBRepBoundPtr VRBRepBound::create() { return VRBRepBoundPtr(new VRBRepBound()); }

void VRBRepBound::addEdge(VRBRepEdgePtr e) { edges.push_back(e); }
void VRBRepBound::setType(string type, bool o) { BRepType = type; outer = o; }

bool VRBRepBound::isOuter() { return outer; }
vector<Vec3d> VRBRepBound::getPoints() { return points; }
vector<VRBRepEdgePtr> VRBRepBound::getEdges() { return edges; }

bool VRBRepBound::isClosed() {
    for (uint i=1; i<edges.size(); i++) {
        if (!edges[i-1]->connectsTo(edges[i])) return false;
    }
    return true;
}

string VRBRepBound::edgeEndsToString() {
    string r;
    for (auto& e : edges) {
        r += " [" + toString(e->EBeg) + " - " + toString(e->EEnd) + "] ";
    }
    return r;
}

bool VRBRepBound::containsNan() {
    for (auto p : points) if (isNan(p)) return true;
    return false;
}

void VRBRepBound::shiftEdges(int i0) {
    vector<VRBRepEdgePtr> shifted;
    for (int i=i0; i<edges.size(); i++) shifted.push_back(edges[i]);
    for (int i=0; i<i0; i++) shifted.push_back(edges[i]);
    edges = shifted;
}

void VRBRepBound::compute() {
    if (edges.size() > 1) {
        if ( sameVec(edges[0]->beg(), edges[1]->beg()) || sameVec(edges[0]->beg(), edges[1]->end()) ) {
            /*cout << "swap edge 0!" << endl;
            cout << " e0p0: " << edges[0].beg() << ", e1p0: " << edges[1].beg() << ", -> " << sameVec(edges[0].beg(), edges[1].beg()) << endl;
            cout << " e0p0: " << edges[0].beg() << ", e1p1: " << edges[1].end() << ", -> " << sameVec(edges[0].beg(), edges[1].end()) << endl;
            cout << " e0p1: " << edges[0].end() << endl;*/
            edges[0]->swap(); // swap first edge
        }

        for (unsigned int i=1; i<edges.size(); i++) {
            auto& e1 = edges[i-1];
            auto& e2 = edges[i];
            if ( sameVec(e2->end(), e1->end()) ) e2->swap();
            if (!sameVec(e1->end(), e2->beg()) ) {
                cout << "Warning in VRSTEP::Bound constructor, edges do not connect! " << e1->end() << " != " << e2->beg() << endl;
                cout << " edge1: " << e1->beg() << " -> " << e1->end() << endl;
                cout << " edge2: " << e2->beg() << " -> " << e2->end() << endl;
            }
        }
    } else {
        if (edges.size() == 0) { cout << "Warning: No bound edges" << endl; return; }
        if ( !sameVec(edges[0]->beg(), edges[0]->end()) ) { cout << "Warning: Single NOT closed edge!" << endl; return; }
    }

    /*if (edges.size() == 1 && edges[0].type == "Circle") {
        if (e.angles.size()) {
            for (auto& a : e.angles) angles.push_back(a);
        }
        return;
    }*/

    for (auto& e : edges) {
        //cout << "edge: " << e.beg() << " " << e.end() << endl;
        for (auto& p : e->points) {
            if (isNan(p)) {
                cout << "Error in VRSTEP::Bound, point contains NaN" << endl;
                continue;
            }
            /*cout << " " << p;
            if (points.size() > 0) cout << " " << sameVec(p, points[points.size()-1]) << " " << sameVec(p, points[0]);
            cout << endl;*/
            if (points.size() > 0) {
                if (sameVec(p, points[points.size()-1])) continue; // same as last point
                //if (sameVec(p, points[0])) continue; // same as first point
            }
            points.push_back(p);
        }
    }

    /*for (auto p : points) {
        cout << "bound point: " << p << endl;
    }*/

    if (points.size() == 0) cout << "Warning1: No bound points" << endl;
}

VRGeometryPtr VRBRepBound::build() {
    VRGeoData boundPoints;

    for (auto p : points) {
        boundPoints.pushVert(p,Vec3d(),Color3f(1,0,0));
        boundPoints.pushPoint();
    }

    for (int i=1; i<points.size(); i++) {
        boundPoints.pushLine(i-1, i);
    }
    boundPoints.pushLine(points.size()-1, 0);

    auto tmp = boundPoints.asGeometry("bounds");
    auto mb = VRMaterial::create("bounds");
    mb->setDiffuse(Color3f(1,0,0));
    mb->setLit(0);
    mb->setPointSize(5);
    mb->setLineWidth(3);
    tmp->setMaterial(mb);

    return tmp;
}



