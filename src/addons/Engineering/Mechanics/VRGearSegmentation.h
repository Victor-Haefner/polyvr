#ifndef VRGEARSEGMENTATION_H_INCLUDED
#define VRGEARSEGMENTATION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "VRMechanismFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct GearVertex;
struct VertexPlane;

class VRGearSegmentation {
    private:
        VRObjectPtr obj;
        Vec3d axis, r1, r2;
        bool isMultiGear = false;
        int fftFreqHint = 1;
        int fftFreqSel = 0;

        vector<VertexPlane> planes;
        vector<vector<double>> gears;
        vector<GearVertex> gearVertices;
        map<int, int> matchedPlanes;

        bool same(double x, double y, double eps);
        double planeEps = 1e-3;
        double matchEps = 1e-3;
        double ringEps = 1e-3;

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
        PosePtr getPolarCoords();

        size_t getNGears();
        vector<double> getGearParams(size_t i);
        VRGeometryPtr createGear(size_t i);

        size_t getNPlanes();
        double getPlanePosition(size_t i);
        vector<Vec2d> getPlaneVertices(size_t i);
        vector<Vec2d> getPlaneContour(size_t i);
        vector<double> getPlaneSineGuess(size_t i, size_t sf);
        vector<double> getPlaneSineApprox(size_t i, size_t sf);
};

OSG_END_NAMESPACE;

#endif // VRGEARSEGMENTATION_H_INCLUDED
