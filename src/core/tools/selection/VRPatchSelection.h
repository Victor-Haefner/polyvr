#ifndef VRPATCHSELECTION_H_INCLUDED
#define VRPATCHSELECTION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <string>
#include "VRSelection.h"
#include "core/math/frustum.h"
#include "core/math/pose.h"
#include "addons/Semantics/Segmentation/VRAdjacencyGraph.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPatchSelection : public VRSelection {
    private:
        map<VRGeometry*, VRAdjacencyGraph> agraphs;
        map<VRGeometry*, int> lastMeshChanges;

        vector<int> crawl(VRGeometryPtr geo, int vertex, float d);

    public:
        VRPatchSelection();

        static shared_ptr<VRPatchSelection> create();

        void select(VRGeometryPtr geo, int vertex, float curvature, int curvNeighbors = 1);
};

OSG_END_NAMESPACE;

#endif // VRPATCHSELECTION_H_INCLUDED
