#include "VRPatchSelection.h"
#include "core/objects/geometry/VRGeometry.h"
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

VRPatchSelection::VRPatchSelection() {}

shared_ptr<VRPatchSelection> VRPatchSelection::create() { return shared_ptr<VRPatchSelection>( new VRPatchSelection() ); }

vector<int> VRPatchSelection::crawl(VRGeometryPtr geo, int vertex, float d) {
    auto compCurv = [&](vector<int>& mask, int p, float d) {
        for (int i : agraph.getNeighbors(p)) {
            if (mask[i] != 1) continue;
            float c0 = agraph.getCurvature(i);
            float c1 = agraph.getCurvature(p);
            if (abs(c1-c0) < d) return true;
        }
        return false;
    };

    auto patchCrawl = [&](vector<int>& inds, vector<int>& brdPnts, vector<int>& mask, float d) {
        vector<int> pool = inds;
        for (int i : inds) {
            auto n = agraph.getNeighbors(i);
            pool.insert(pool.end(), n.begin(), n.end());
        }

        vector<int> newPnts;
        for (int p : pool) {
            if (mask[p] != 0) continue;
            if (compCurv(mask, p, d)) {
                mask[p] = 1;
                newPnts.push_back(p);
                continue;
            }
            mask[p] = 2;
            brdPnts.push_back(p);
        }
        return newPnts;
    };

	int N = geo->getMesh()->getPositions()->size();
	vector<int> mask(N, 0);
	vector<int> brdPnts;
	vector<int> newPnts;
	newPnts.push_back(vertex);
	vector<int> patch = newPnts;
    for (int p : newPnts) mask[p] = 1;
    while (newPnts.size() > 0) {
        newPnts = patchCrawl(newPnts, brdPnts, mask, d);
        patch.insert(patch.end(), newPnts.begin(), newPnts.end());
    }
    patch.insert(patch.end(), brdPnts.begin(), brdPnts.end());
    return patch;
}

void VRPatchSelection::select(VRGeometryPtr geo, int vertex, float curvature, int curvNeighbors) {
    agraph.clear();
    agraph.setGeometry(geo);
	agraph.compNeighbors();
	agraph.compCurvatures(curvNeighbors);

    selection_atom patch;
    patch.geo = geo;
    patch.subselection = crawl(geo, vertex, curvature);
    selected[geo.get()] = patch;
}
