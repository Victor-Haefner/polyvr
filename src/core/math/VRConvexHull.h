#ifndef VRCONVEXHULL_H_INCLUDED
#define VRCONVEXHULL_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRConvexHull {
    private:

    public:
        VRConvexHull();
        ~VRConvexHull();

        VRGeometryPtr compute(VRGeometryPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRCONVEXHULL_H_INCLUDED
