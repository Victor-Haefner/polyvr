#ifndef VRSEGMENTATION_H_INCLUDED
#define VRSEGMENTATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRGeometry;

enum SEGMENTATION_ALGORITHM {
    HOUGH = 0
};

class VRSegmentation {
    private:
        VRSegmentation();

    public:

        static VRObject* extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta);

        static void removeDuplicates(VRGeometry* geo);
        static void fillHoles(VRGeometry* geo);

        static VRObject* convexDecompose(VRGeometry* geo);
};

OSG_END_NAMESPACE;

#endif // VRSEGMENTATION_H_INCLUDED
