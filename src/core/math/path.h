#ifndef path_H_INCLUDED
#define path_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "pose.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class path : public VRStorage {
    private:
        vector<pose> points;
        vector<Vec3f> point_colors;

        int degree = 3;
        int direction = 1;
        int iterations = 80;
        bool closed = false;

        vector<Vec3f> positions;
        vector<Vec3f> directions;
        vector<Vec3f> up_vectors;
        vector<Vec3f> colors;

        Vec3f interp(vector<Vec3f>& vec, float t, int i = 0, int j = 0);
        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);
        void cubicBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void quadraticBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f n);
        void linearBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1);

    public:
        path(int degree = 3);
        ~path();

        static shared_ptr<path> create();

        int addPoint( const pose& p, Vec3f c = Vec3f() );
        void setPoint(int i, const pose& p, Vec3f c = Vec3f() );
        pose& getPoint(int i);
        Vec3f getPointColor(int i);
        vector<pose> getPoints();

        void invert();
        void close();
        bool isClosed();
        void compute(int N);
        vector<Vec3f> getPositions();
        vector<Vec3f> getDirections();
        vector<Vec3f> getUpvectors();
        vector<Vec3f> getColors();
        Vec3f getPosition(float t, int i = 0, int j = 0);
        void getOrientation(float t, Vec3f& dir, Vec3f& up, int i = 0, int j = 0);
        Vec3f getColor(float t, int i = 0, int j = 0);
        pose getPose(float t, int i = 0, int j = 0);

        float getClosestPoint(Vec3f p); // return t parameter on path
        float getDistance(Vec3f p);
        vector<float> computeInflectionPoints(int i, int j);

        void approximate(int degree);

        float getLength();
        int size();

        void update();
        void clear();
};

OSG_END_NAMESPACE;

#endif // path_H_INCLUDED
