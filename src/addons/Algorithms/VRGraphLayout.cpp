#include "VRGraphLayout.h"
#include "core/math/Octree.h"

using namespace OSG;

VRGraphLayout::VRGraphLayout() {}

VRGraphLayoutPtr VRGraphLayout::create() { return VRGraphLayoutPtr( new VRGraphLayout() ); }

void VRGraphLayout::setGraph(graph_basePtr g) { graph = g; }
graph_basePtr VRGraphLayout::getGraph() { return graph; }
void VRGraphLayout::setAlgorithm(ALGORITHM a, int p) { algorithms[p] = a; }
void VRGraphLayout::clearAlgorithms() { algorithms.clear(); }
void VRGraphLayout::setAlgorithm(string a, int p) {
    ALGORITHM A = SPRINGS;
    if (a == "OCCUPANCYMAP") A = OCCUPANCYMAP;
    setAlgorithm(A, p);
}

void VRGraphLayout::applySprings(float eps) {
    if (!graph) return;
    for (auto& n : graph->getEdges()) {
        for (auto& e : n) {
            auto f1 = getFlag(e.from);
            auto f2 = getFlag(e.to);
            auto& n1 = graph->getNode(e.from);
            auto& n2 = graph->getNode(e.to);
            Vec3f p1 = n1.box.center();
            Vec3f p2 = n2.box.center();

            float r = radius + n1.box.radius() + n2.box.radius();
            Vec3f d = p2 - p1;
            float x = (d.length() - r)*eps; // displacement
            d.normalize();
            if (abs(x) < eps) continue;
            if (x < -r*eps) x = -r*eps; // numerical safety ;)

            Vec3f grav = gravity*x*0.1; // TODO: not yet working!
            p1 += d*x*r*0.2 + grav;
            p2 += -d*x*r*0.2 + grav;

            switch (e.connection) {
                case graph_base::SIMPLE:
                    if (f1 != FIXED) graph->setPosition(e.from, p1);
                    if (f2 != FIXED) graph->setPosition(e.to, p2);
                    break;
                case graph_base::HIERARCHY:
                    if (f2 != FIXED) graph->setPosition(e.to, p2);
                    else if (f1 != FIXED) graph->setPosition(e.from, p1);
                    break;
                case graph_base::DEPENDENCY:
                    if (f1 != FIXED) graph->setPosition(e.from, p1);
                    else if (f2 != FIXED) graph->setPosition(e.to, p2);
                    break;
                case graph_base::SIBLING:
                    if (x < 0) { // push away siblings
                        if (f1 != FIXED) graph->setPosition(e.from, p1);
                        if (f2 != FIXED) graph->setPosition(e.to, p2);
                    }
                    break;
            }
        }
    }
}

void VRGraphLayout::applyOccupancy(float eps) {
    if (!graph) return;
    auto& nodes = graph->getNodes();
    Octree o(10*eps);

    for (unsigned long i=0; i<nodes.size(); i++) {
        auto& n = nodes[i];
        //o.add( n.box.center(), (void*)i );
        o.addBox( n.box, (void*)i );
    }

    for (int i=0; i<nodes.size(); i++) {
        auto& n = nodes[i];
        Vec3f pn = n.box.center();
        float rs = radius + 2*n.box.radius();
        if ( getFlag(i) == FIXED ) continue;

        float s = 1.0;
        Vec3f D;
        for (auto& on2 : o.boxSearch(n.box) ) {
            int j = (long)on2;
            if (i == j) continue; // no self interaction

            auto& n2 = nodes[j];
            Vec3f d = n2.box.center() - pn;
            Vec3f w = n.box.size() + n2.box.size();

            Vec3f vx = Vec3f(abs(d[0]), abs(d[1]), abs(d[2])) - w*0.5;
            float x = vx[0]; // get smallest intrusion
            int dir = 0;
            if (abs(vx[1]) < abs(x)) { x = vx[1]; dir = 1; }
            if (abs(vx[2]) < abs(x)) { x = vx[2]; dir = 2; }

            //cout << " dir " << dir << "   x " << x << "   d " << d << "   w2 " << w*0.5 << "   vx " << vx << endl;
            //cout << "displ " << i << " " << j << " w " << n.box.size() << " + " << n2.box.size() << "    d " << d << "    x " << x << endl;
            d.normalize();
            D[dir] -= d[dir]*abs(x);//*0.15;
        }

        graph->setPosition(i, pn+D); // move node away from neighbors
    }
}

void VRGraphLayout::compute(int N, float eps) {
    if (!graph) return;
    for (int i=0; i<N; i++) { // steps
        for (auto a : algorithms) {
            switch(a.second) {
                case SPRINGS:
                    applySprings(eps);
                    break;
                case OCCUPANCYMAP:
                    applyOccupancy(eps);
                    break;
            }
        }
    }

    // update graph nodes a second time
    for (int i=0; i<graph->getNodes().size(); i++) graph->update(i, false);
}

VRGraphLayout::FLAG VRGraphLayout::getFlag(int i) { return flags.count(i) ? flags[i] : NONE; }
void VRGraphLayout::fixNode(int i) { flags[i] = FIXED; }

void VRGraphLayout::setRadius(float r) { radius = r; }
void VRGraphLayout::setGravity(Vec3f v) { gravity = v; }
