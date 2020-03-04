#ifndef VRSEGMENTATION_H_INCLUDED
#define VRSEGMENTATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include <vector>
#include <map>

#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

enum SEGMENTATION_ALGORITHM {
    HOUGH = 0
};

struct Vertex;
struct Edge;
struct Triangle;
struct Border;

class VRSegmentation {
    private:
        VRSegmentation();

    public:

        static VRObjectPtr extractPatches(VRGeometryPtr geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3d normal, Vec3d normal_delta);
        static vector<int> growPatch(VRGeometryPtr geo, int i);

        static void removeDuplicates(VRGeometryPtr geo);
        static void fillHoles(VRGeometryPtr geo, int steps);

        static VRObjectPtr convexDecompose(VRGeometryPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRSEGMENTATION_H_INCLUDED
