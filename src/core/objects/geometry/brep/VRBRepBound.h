#ifndef VRBREPBOUND_H_INCLUDED
#define VRBREPBOUND_H_INCLUDED

#include "VRBRepUtils.h"
#include "VRBRepEdge.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepBound : public VRBRepUtils {
    protected:
        vector<VRBRepEdgePtr> edges;
        vector<Vec3d> points;
        vector<float> angles;
        bool outer = true;
        string BRepType;

    public:
        VRBRepBound();
        ~VRBRepBound();

        static VRBRepBoundPtr create();

        void addEdge(VRBRepEdgePtr e);
        void setType(string type, bool outer);

        bool isClosed();
        bool isOuter();
        vector<Vec3d> getPoints();
        vector<VRBRepEdgePtr> getEdges();
        string edgeEndsToString();

        bool containsNan();
        void shiftEdges(int i0);

        void compute();
        VRGeometryPtr build();
};

OSG_END_NAMESPACE;

#endif // VRBREPBOUND_H_INCLUDED
