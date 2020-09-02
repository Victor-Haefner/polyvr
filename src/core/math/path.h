#ifndef Path_H_INCLUDED
#define Path_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"
#include "pose.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Path : public VRStorage {
    private:
        vector<Pose> points;
        vector<Color3f> point_colors;

        vector<Vec3d> controlPoints;

        int degree = 3;
        int direction = 1;
        int iterations = 80;
        bool closed = false;

        vector<Vec3d> positions;
        vector<Vec3d> directions;
        vector<Vec3d> up_vectors;
        vector<Vec3d> colors;

        Vec3d interp(vector<Vec3d>& vec, float t, int i = 0, int j = 0);
        Vec3d projectInPlane(Vec3d v, Vec3d n, bool keep_length);
        void cubicBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1, Vec3d n0, Vec3d n1);
        void quadraticBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1, Vec3d n);
        void linearBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1);
        void computeUpVectors(Vec3d* container, Vec3d* dirs, int N, Vec3d p0, Vec3d p1);

    public:
        Path(int degree = 3);
        ~Path();

        static PathPtr create();

        void set(PosePtr p1, PosePtr p2, int res);
        int addPoint( Pose p, Color3f c = Color3f() );
        int addPoint2( Vec3d p, Vec3d d, Color3f c, Vec3d u );
        void setPoint(int i, const Pose& p, Color3f c = Color3f() );
        Pose& getPoint(int i);
        Color3f getPointColor(int i);
        void setPointColor(int i, Color3f c);
        vector<Pose> getPoints();
        vector<Vec3d> getControlPoints();

        void invert();
        void close();
        bool isClosed();
        void compute(int N);
        vector<Vec3d> getPositions();
        vector<Vec3d> getDirections();
        vector<Vec3d> getUpVectors();
        vector<Vec3d> getColors();
        vector<Pose> getPoses();
        Vec3d getPosition(float t, int i = 0, int j = 0, bool fast = true);
        void getOrientation(float t, Vec3d& dir, Vec3d& up, int i = 0, int j = 0, bool fast = true);
        Color3f getColor(float t, int i = 0, int j = 0);
        PosePtr getPose(float t, int i = 0, int j = 0, bool fast = true);

        float getClosestPoint(Vec3d p); // return t parameter on Path
        float getDistance(Vec3d p);
        float getDistanceToHull(Vec3d p);
        vector<double> computeInflectionPoints(int i = 0, int j = 0, float threshold = 1e-9, float accelerationThreshold = 0, Vec3i axis = Vec3i(1,1,1));

        bool isStraight(int i = 0, int j = 0);
        bool isCurve(int i = 0, int j = 0);
        bool isSinuous(int i = 0, int j = 0);
        bool isCrossing(PathPtr path);

        void approximate(int degree);
        void translate(Vec3d t);

        float getLength(int i = 0, int j = 0);
        int size();

        void update();
        void clear();
};

OSG_END_NAMESPACE;

#endif // Path_H_INCLUDED
