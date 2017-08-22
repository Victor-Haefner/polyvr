#ifndef path_H_INCLUDED
#define path_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"
#include "pose.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class path : public VRStorage {
    private:
        vector<pose> points;
        vector<Color3f> point_colors;

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

    public:
        path(int degree = 3);
        ~path();

        static shared_ptr<path> create();

        int addPoint( const pose& p, Color3f c = Color3f() );
        void setPoint(int i, const pose& p, Color3f c = Color3f() );
        pose& getPoint(int i);
        Color3f getPointColor(int i);
        vector<pose> getPoints();

        void invert();
        void close();
        bool isClosed();
        void compute(int N);
        vector<Vec3d> getPositions();
        vector<Vec3d> getDirections();
        vector<Vec3d> getUpvectors();
        vector<Vec3d> getColors();
        vector<pose> getPoses();
        Vec3d getPosition(float t, int i = 0, int j = 0, bool fast = true);
        void getOrientation(float t, Vec3d& dir, Vec3d& up, int i = 0, int j = 0, bool fast = true);
        Color3f getColor(float t, int i = 0, int j = 0);
        pose getPose(float t, int i = 0, int j = 0, bool fast = true);

        float getClosestPoint(Vec3d p); // return t parameter on path
        float getDistance(Vec3d p);
        vector<double> computeInflectionPoints(int i = 0, int j = 0, float threshold = 1e-9, Vec3i axis = Vec3i(1,1,1));

        bool isStraight(int i = 0, int j = 0);
        bool isCurve(int i = 0, int j = 0);
        bool isSinuous(int i = 0, int j = 0);

        void approximate(int degree);

        float getLength(int i = 0, int j = 0);
        int size();

        void update();
        void clear();
};

OSG_END_NAMESPACE;

#endif // path_H_INCLUDED
