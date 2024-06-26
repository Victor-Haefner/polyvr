#ifndef VRGEARSEGMENTATION_H_INCLUDED
#define VRGEARSEGMENTATION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "VRMechanismFwd.h"
#include "VRPolarVertex.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VertexPlane;

class VRGearSegmentation {
    private:
        VRObjectPtr obj;
        Vec3d axis, r1, r2;
        Vec3d axisOffset;
        bool isMultiGear = false;
        int fftFreqHint = 1;
        int fftFreqSel = 0;

        vector<VertexPlane> planes;
        vector<vector<double>> gears;
        vector<PolarVertex> gearVertices;
        map<int, int> matchedPlanes;

        bool same(double x, double y, double eps);
        double planeEps = 1e-3;
        double matchEps = 1e-3;
        double ringEps = 1e-3;

        void cleanup();
        void computeAxis();
        void computePolarVertices();
        void computePlanes();
        void groupPlanes();
        void computeRings();
        void computeContours();
        void computeSineApprox();
        void computeGearParams(int fN);

    public:
        VRGearSegmentation();
        ~VRGearSegmentation();
        static VRGearSegmentationPtr create();

        void setBinSizes(double planeEps, double matchEps, double ringEps);
        void setFFTFreqHint(int h, int s);

        void analyse(VRObjectPtr obj);
        void runTest();
        void printResults();

        Vec3d getAxis();
        Vec3d getAxisOffset();
        PosePtr getPolarCoords();

        size_t getNGears();
        vector<double> getGearParams(size_t i);
        VRGeometryPtr createGear(size_t i);

        size_t getNPlanes();
        double getPlanePosition(size_t i);
        vector<Vec2d> getPlaneVertices(size_t i);
        vector<Vec2d> getPlaneContour(size_t i);
        vector<double> getPlaneFrequencies(size_t i);
        vector<double> getPlaneSineGuess(size_t i, size_t sf = 0);
        vector<double> getPlaneSineApprox(size_t i, size_t sf = 0);

        VRTransformPtr getContourViz();
        VRTransformPtr getSineFitViz(int precision = 360);
};

OSG_END_NAMESPACE;

#endif // VRGEARSEGMENTATION_H_INCLUDED
