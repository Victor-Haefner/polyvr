#ifndef path_H_INCLUDED
#define path_H_INCLUDED

#include <OpenSG/OSGGeometry.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class path {
    private:
        // end points and normals
        Vec3f ep1;
        Vec3f ep2;
        Vec3f n1;
        Vec3f n2;
        Vec3f u1;
        Vec3f u2;
        Vec3f c1;
        Vec3f c2;

        int iterations = 80;

        vector<Vec3f> points;
        vector<Vec3f> directions;
        vector<Vec3f> up_vectors;
        vector<Vec3f> colors;

        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);
        void calcBezRowOld(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void cubicBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void quadraticBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n);
        void linearBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1);

    public:
        path();

        void setStartPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u = Vec3f(0,1,0));
        void setEndPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u = Vec3f(0,1,0));
        void getStartPoint(Vec3f& p, Vec3f& n, Vec3f& c);
        void getEndPoint(Vec3f& p, Vec3f& n, Vec3f& c);
        void invert();
        void compute(int N);
        vector<Vec3f> getPositions();
        vector<Vec3f> getDirections();
        vector<Vec3f> getUpvectors();
        vector<Vec3f> getColors();
        Vec3f getPosition(float t);
        void getOrientation(float t, Vec3f& dir, Vec3f& up);
        Vec3f getColor(float t);

        void update();
};

OSG_END_NAMESPACE;

#endif // path_H_INCLUDED
