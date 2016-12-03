#include "VRAdjacencyGraph.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"

#include <OpenSG/OSGTriangleIterator.h>

using namespace OSG;
using namespace std;

VRAdjacencyGraph::VRAdjacencyGraph() {}

shared_ptr<VRAdjacencyGraph> VRAdjacencyGraph::create() { return shared_ptr<VRAdjacencyGraph>(new VRAdjacencyGraph()); }

void VRAdjacencyGraph::setGeometry(VRGeometryPtr geo) { this->geo = geo; }

void VRAdjacencyGraph::clear() {
    edge_triangle_loockup.clear();
    vertex_neighbor_params.clear();
    vertex_neighbors.clear();
    vertex_curvatures.clear();
    geo.reset();
}

void VRAdjacencyGraph::compNeighbors() {
    vertex_neighbor_params.clear();
    vertex_neighbors.clear();
    auto sgeo = geo.lock();
    if (!sgeo) return;
    map< int, map<int, int> > tmpdict;

    for (TriangleIterator it = TriangleIterator(sgeo->getMesh()->geo); !it.isAtEnd(); ++it) {
        int i0 = it.getPositionIndex(0);
        int i1 = it.getPositionIndex(1);
        int i2 = it.getPositionIndex(2);

        auto reg = [&](int j0, int j1, int j2) {
            if ( tmpdict.count(j0) == 0 ) tmpdict[j0] = map<int, int>();
            if ( tmpdict[j0].count(j1) == 0 ) tmpdict[j0][j1] = 0;
            if ( tmpdict[j0].count(j2) == 0 ) tmpdict[j0][j2] = 0;
            tmpdict[j0][j1] += 1;
            tmpdict[j0][j2] += 1;
        };

        reg(i0,i1,i2);
        reg(i1,i0,i2);
        reg(i2,i0,i1);
    }

    int N = sgeo->getMesh()->geo->getPositions()->size();
    vertex_neighbor_params.resize(2*N);
    for (auto i : tmpdict) vertex_neighbor_params[2*i.first+1] = i.second.size();

    int kN = 0;
    for (int i = 0; i < N; i++) {
        vertex_neighbor_params[2*i] = kN;
        kN += vertex_neighbor_params[2*i+1];
    }

    vertex_neighbors.resize(kN);
    for (auto i : tmpdict) {
        int o = vertex_neighbor_params[2*i.first];
        int k = 0;
        for (auto j : i.second) { vertex_neighbors[o+k] = j.first; k++; }
    }
}

void VRAdjacencyGraph::compCurvatures(int range) {
    vertex_curvatures.clear();
    auto sgeo = geo.lock();
    if (!sgeo) return;

    auto pos = sgeo->getMesh()->geo->getPositions();
    auto norms = sgeo->getMesh()->geo->getNormals();
    int N = pos->size();

    /*auto curvMax = [&](int i, int range) {
		Vec3f n = norms->getValue<Vec3f>(i);
		Vec3f vi = pos->getValue<Vec3f>(i);
		float K = 0;
		float Kmax = 0;
		auto Ne = getNeighbors(i,range);
		if (Ne.size() == 0) return K;

		for (int j : Ne) {
			if (j >= N) continue;
			Vec3f d = pos->getValue<Vec3f>(j) - vi;
			float k = 2*n.dot(d)/d.squareLength();
			if (abs(k) > Kmax) {
                K = k;
                Kmax = abs(k);
			}
		}

		return K;
    };*/

    auto curvAvg = [&](int i, int range) {
		Vec3f n = norms->getValue<Vec3f>(i);
		Vec3f vi = pos->getValue<Vec3f>(i);
		float K = 0;
		auto Ne = getNeighbors(i,range);
		if (Ne.size() == 0) return K;

		for (int j : Ne) {
			if (j >= N) continue;
			Vec3f d = pos->getValue<Vec3f>(j) - vi;
			K += 2*n.dot(d)/d.length();
		}

		K /= Ne.size();
		return K;
    };

    vertex_curvatures.resize(N);
    for (int i = 0; i < N; i++) vertex_curvatures[i] = curvAvg(i,range);
}

void VRAdjacencyGraph::compTriLoockup() {
    edge_triangle_loockup.clear();
    auto sgeo = geo.lock();
    if (!sgeo) return;

    for (TriangleIterator it = TriangleIterator(sgeo->getMesh()->geo); !it.isAtEnd(); ++it) {
        triangle t;
        t.v1 = it.getPositionIndex(0);
        t.v2 = it.getPositionIndex(1);
        t.v3 = it.getPositionIndex(2);

        auto reg = [&](int j0, int j1) {
            if (edge_triangle_loockup.count(j0) == 0) edge_triangle_loockup[j0] = map<int, vector<triangle> >();
            if (edge_triangle_loockup[j0].count(j1) == 0) edge_triangle_loockup[j0][j1] = vector<triangle>();
            edge_triangle_loockup[j0][j1].push_back(t);
        };

        reg(t.v1,t.v2); reg(t.v2,t.v1);
        reg(t.v1,t.v3); reg(t.v3,t.v1);
        reg(t.v2,t.v3); reg(t.v3,t.v2);
    }
}

vector<int> VRAdjacencyGraph::getNeighbors(int i, int range) {
    auto sgeo = geo.lock();
    if (!sgeo) return vector<int>();
    if (2*i+1 >= int(vertex_neighbor_params.size())) return vector<int>();

    vector<int> job;
    vector<int> nextJob;
    vector<int> res;
    map<int, bool> resMask;
    job.push_back(i);
    while (range > 0) {
        nextJob.clear();
        for (auto j : job) {
            if (resMask.count(j)) continue;
            resMask[j] = true;
            int o = vertex_neighbor_params[2*j];
            int nN = vertex_neighbor_params[2*j+1];
            for (int k=0; k<nN; k++) {
                int n = vertex_neighbors[o+k];
                if (resMask.count(n)) continue;
                nextJob.push_back(n);
                res.push_back(n);
            }
        }
        job = nextJob;
        range--;
    }

    return res;
}

vector<int> VRAdjacencyGraph::getBorderVertices() {
    vector<int> borders;

    for (auto i : edge_triangle_loockup) {
        for (auto j : i.second) {
            if (j.second.size() != 1) continue; // not an edge
            borders.push_back(i.first);
            borders.push_back(j.first);
        }
    }

    return borders;
}

float VRAdjacencyGraph::getCurvature(int i) {
    auto sgeo = geo.lock();
    if (!sgeo) return 0;
    if (i > int(vertex_curvatures.size()) || i < 0) return 0;
    return vertex_curvatures[i];
}


