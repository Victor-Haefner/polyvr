#ifndef patch_H_INCLUDED
#define patch_H_INCLUDED

#include <OpenSG/OSGGeometry.h>
#include "path.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class patch {
    public:
        template <int S>
        struct bezPolygon {
            Vec3f* p;// EckPunkte
            Vec3f* n;// Normale
            Vec2f* tex;// Normale

            GeometryMTRecPtr geo;// eigentliche Geometrie

            int N;//auflösung
            bool wired;

            bezPolygon();
            ~bezPolygon();
            void initPolygon();
        };

        // instanciate the template
        bezPolygon<1> bp1;
        bezPolygon<2> bp2;
        bezPolygon<3> bp3;
        bezPolygon<4> bp4;

    private:
        Vec3f projectInPlane(Vec3f v, Vec3f n, bool keep_length);

        Vec3f reflectInPlane(Vec3f v, Vec3f n);

        GeometryMTRecPtr makeTrianglePlaneGeo(int N, bool wire = false);

        NodeMTRecPtr makeTrianglePlane(int N, bool wire = false);

        void calcBezQuadPlane(bezPolygon<4>& q);

        void calcBezTrianglePlane(bezPolygon<3>& q);

    public:
        patch() {}

        //iteriert über die flächen der geometrie und macht bezierflächen hin
        NodeMTRecPtr applyBezierOnGeometry(GeometryMTRecPtr geo, int N);
};

OSG_END_NAMESPACE;

#endif // patch_H_INCLUDED
