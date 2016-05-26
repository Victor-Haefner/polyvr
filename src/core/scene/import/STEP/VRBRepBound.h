#ifndef VRBREPBOUND_H_INCLUDED
#define VRBREPBOUND_H_INCLUDED

#include "VRBRepUtils.h"
#include "VRBRepEdge.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepBound : public VRBRepUtils {
    public:
        vector<VRBRepEdge> edges;
        vector<Vec3f> points;
        bool outer = true;
        string BRepType;

    public:
        VRBRepBound();

        bool isClosed();
};

OSG_END_NAMESPACE;

#endif // VRBREPBOUND_H_INCLUDED
