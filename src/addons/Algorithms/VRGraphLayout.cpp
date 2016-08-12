#include "VRGraphLayout.h"

using namespace OSG;

VRGraphLayout::VRGraphLayout() {}

void VRGraphLayout::setGraph(graph<Vec3f>& g) { this->g = g; }
graph<Vec3f>& VRGraphLayout::getGraph() { return g; }
void VRGraphLayout::setAlgorithm(ALGORITHM a) { algorithm = a; }

void VRGraphLayout::applySprings(int N, float eps) {
    //float eps = 1.0/N;

    for (int i=0; i<N; i++) { // steps
        for (auto& n : g.getEdges()) {
            for (auto& e : n) {
                auto f1 = getFlag(e.from);
                auto f2 = getFlag(e.to);
                Vec3f& n1 = g.getNodes()[e.from];
                Vec3f& n2 = g.getNodes()[e.to];
                Vec3f d = n2-n1;
                float x = (d.length() - radius)*eps; // displacement
                if (abs(x) < eps) continue;
                Vec3f g; // TODO: not yet working!
                g = gravity*x*0.1;
                if (f2 != FIXED) n2 += -d*x + g;
                else if (f1 != FIXED) n1 += d*x + g;
            }
        }
    }
}

void VRGraphLayout::compute(int N, float eps) {
    switch(algorithm) {
        case SPRINGS:
            applySprings(N, eps);
            break;
    }
}

VRGraphLayout::FLAG VRGraphLayout::getFlag(int i) { return flags.count(i) ? flags[i] : NONE; }
void VRGraphLayout::fixNode(int i) { flags[i] = FIXED; }

void VRGraphLayout::setRadius(float r) { radius = r; }
void VRGraphLayout::setGravity(Vec3f v) { gravity = v; }
