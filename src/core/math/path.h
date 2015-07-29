#ifndef path_H_INCLUDED
#define path_H_INCLUDED

#include <OpenSG/OSGGeometry.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;

class path {
    public:
        // points
        struct pnt {
            Vec3f p,n,c,u;
            pnt(Vec3f p, Vec3f n, Vec3f c, Vec3f u);
        };

    private:
        vector<pnt> points;

        int direction = 1;
        int iterations = 80;
        bool closed = false;

        vector<Vec3f> positions;
        vector<Vec3f> directions;
        vector<Vec3f> up_vectors;
        vector<Vec3f> colors;

        Vec3f interp(vector<Vec3f>& vec, float t);
        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);
        void cubicBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void quadraticBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f n);
        void linearBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1);

    public:
        path();

        int addPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u = Vec3f(0,1,0));
        int addPoint(VRTransform* t);
        void setPoint(int i, Vec3f p, Vec3f n, Vec3f c, Vec3f u = Vec3f(0,1,0));
        pnt getPoint(int i);
        vector<pnt> getPoints();

        void invert();
        void close();
        bool isClosed();
        void compute(int N);
        vector<Vec3f> getPositions();
        vector<Vec3f> getDirections();
        vector<Vec3f> getUpvectors();
        vector<Vec3f> getColors();
        Vec3f getPosition(float t);
        void getOrientation(float t, Vec3f& dir, Vec3f& up);
        Vec3f getColor(float t);

        float getLength();
        int size();

        void update();
        void clear();
};

OSG_END_NAMESPACE;

#endif // path_H_INCLUDED
