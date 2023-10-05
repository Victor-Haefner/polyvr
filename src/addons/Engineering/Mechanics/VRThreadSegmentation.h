#ifndef VRThreadSegmentation_H_INCLUDED
#define VRThreadSegmentation_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "VRMechanismFwd.h"
#include "VRPolarVertex.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRThreadSegmentation {
    private:
        VRObjectPtr obj;
        double length = 0;
        double radius = 0;
        double pitch = 0;
        double midOffset = 0;
        double planeEps = 1e-3;

        vector<PolarVertex> axleVertices;
        vector<vector<double>> axleParams;
        vector<Vec2d> sineFitInput;
        SineFitPtr sineFit;
        int chosenFreqI = 0;

        Vec3d axis;
        Vec3d axisOffset;
        Vec3d r1;
        Vec3d r2;

        void computeAxis();
        void computePolarVertices();

    public:
        VRThreadSegmentation();
        ~VRThreadSegmentation();
        static VRThreadSegmentationPtr create();

        void setBinSizes(double planeEps);
        void setPitch(double p);

        void analyse(VRObjectPtr obj);

        VRGeometryPtr createThread();
        VRTransformPtr getProfileViz();
        VRGeometryPtr getSineFitViz(int precision = 100);

        vector<Vec2d> getProfile();
        double getRadius();
        double getLength();
        Vec3d getAxis();
        Vec3d getAxisOffset();
};

OSG_END_NAMESPACE;

#endif // VRThreadSegmentation_H_INCLUDED
