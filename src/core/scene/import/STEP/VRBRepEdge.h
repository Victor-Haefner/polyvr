#ifndef VRBREPEDGE_H_INCLUDED
#define VRBREPEDGE_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "VRBRepUtils.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepEdge : public VRBRepUtils {
    public:
        vector<Vec3f> points;
        Vec3f n;

        VRBRepEdge();

        Vec3f& beg();
        Vec3f& end();
        void swap();
        bool connectsTo(VRBRepEdge& e);
};

OSG_END_NAMESPACE;

#endif // VRBREPEDGE_H_INCLUDED
