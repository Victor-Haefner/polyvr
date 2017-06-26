#ifndef patch_H_INCLUDED
#define patch_H_INCLUDED

#include <OpenSG/OSGGeometry.h>
#include "path.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class patch {
    public:
        template <int S>
        struct bezVRPolygon {
            Vec3f* p;// EckPunkte
            Vec3f* n;// Normale
            Vec2f* tex;// Normale

            GeometryMTRecPtr geo;// eigentliche Geometrie

            int N;//auflösung
            bool wired;

            bezVRPolygon();
            ~bezVRPolygon();
            void initVRPolygon();
        };

        // instanciate the template
        bezVRPolygon<1> bp1;
        bezVRPolygon<2> bp2;
        bezVRPolygon<3> bp3;
        bezVRPolygon<4> bp4;

    private:
        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);

        Vec3f reflectInPlane(Vec3f v, Vec3f n);

        GeometryMTRecPtr makeTrianglePlaneGeo(int N, bool wire = false);

        NodeMTRecPtr makeTrianglePlane(int N, bool wire = false);

        void calcBezQuadPlane(bezVRPolygon<4>& q);

        void calcBezTrianglePlane(bezVRPolygon<3>& q);

    public:
        patch() {}

        //iteriert über die flächen der geometrie und macht bezierflächen hin
        NodeMTRecPtr applyBezierOnGeometry(GeometryMTRecPtr geo, int N);
};

OSG_END_NAMESPACE;

#endif // patch_H_INCLUDED
