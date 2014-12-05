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
        Vec3f c1;
        Vec3f c2;

        vector<Vec3f> points;
        vector<Vec3f> normals;
        vector<Vec3f> colors;

        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);
        void calcBezRowOld(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void cubicBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1);
        void quadraticBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n);
        void linearBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1);

    public:
        path();

        void setStartPoint(Vec3f p, Vec3f n, Vec3f c);
        void setEndPoint(Vec3f p, Vec3f n, Vec3f c);
        void getStartPoint(Vec3f& p, Vec3f& n, Vec3f& c);
        void getEndPoint(Vec3f& p, Vec3f& n, Vec3f& c);
        void compute(int N);
        vector<Vec3f> getPositions();
        vector<Vec3f> getNormals();
        vector<Vec3f> getColors();
        Vec3f getPosition(float t);
        Vec3f getNormal(float t);
        Vec3f getColor(float t);

        void update();
};

OSG_END_NAMESPACE;

#endif // path_H_INCLUDED
