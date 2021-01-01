#ifndef VRGEARSEGMENTATION_H_INCLUDED
#define VRGEARSEGMENTATION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/VRObjectFwd.h"
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

        vector<VertexPlane> planes;
        vector<vector<double>> gears;
        vector<GearVertex> gearVertices;
        map<int, int> matchedPlanes;

        bool same(double x, double y, double eps);

        void computeAxis();
        void computePolarVertices();
        void computePlanes();
        void groupPlanes();
        void computeRings();
        void computeContours();
        void computeSineApprox();
        void computeGearParams();

    public:
        VRGearSegmentation();
        ~VRGearSegmentation();
        static VRGearSegmentationPtr create();

        void analyse(VRObjectPtr obj);
        void runTest();
        void printResults();


        Vec3d getAxis();

        int getNGears();
        vector<double> getGearParams(int i);

        int getNPlanes();
        double getPlanePosition(int i);
        vector<Vec2d> getPlaneContour(int i);
        vector<double> getPlaneSineGuess(int i);
        vector<double> getPlaneSineApprox(int i);
};

OSG_END_NAMESPACE;

#endif // VRGEARSEGMENTATION_H_INCLUDED
