#ifndef VRAXLESEGMENTATION_H_INCLUDED
#define VRAXLESEGMENTATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "VRMechanismFwd.h"
#include "VRPolarVertex.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAxleSegmentation {
    private:
        VRObjectPtr obj;
        double length = 0;
        double radius = 0;
        double midOffset = 0;
        double planeEps = 1e-3;

        vector<PolarVertex> axleVertices;
        vector<vector<double>> axleParams;

        Vec3d axis;
        Vec3d axisOffset;
        Vec3d r1;
        Vec3d r2;

        void computeAxis();
        void computePolarVertices();

    public:
        VRAxleSegmentation();
        ~VRAxleSegmentation();
        static VRAxleSegmentationPtr create();

        void setBinSizes(double planeEps);

        void analyse(VRObjectPtr obj);

        VRGeometryPtr createAxle();
        VRTransformPtr getProfileViz();

        vector<Vec2d> getProfile();
        double getRadius();
        double getLength();
        Vec3d getAxis();
        Vec3d getAxisOffset();
};

OSG_END_NAMESPACE;

#endif // VRAXLESEGMENTATION_H_INCLUDED
