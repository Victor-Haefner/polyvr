#ifndef VRBREPSURFACE_H_INCLUDED
#define VRBREPSURFACE_H_INCLUDED

#include "VRBRepUtils.h"
#include "VRBRepBound.h"

#include "core/math/pose.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepSurface : public VRBRepUtils {
    public:
        vector<VRBRepBound> bounds;
        pose trans;
        double R = 1;

    public:
        VRBRepSurface();

        VRGeometryPtr build(string type);
};

OSG_END_NAMESPACE;

#endif // VRBREPSURFACE_H_INCLUDED
