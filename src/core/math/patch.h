#ifndef patch_H_INCLUDED
#define patch_H_INCLUDED

#include "path.h"
#include "VRMathFwd.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Patch {
    public:
        template <int S>
        struct bezVRPolygon {
            vector<Vec3d> p;// EckPunkte
            vector<Vec3d> n;// Normale
            vector<Vec2d> tex;// Normale

            VRGeometryPtr geo;// eigentliche Geometrie

            int N = 0;//auflösung
            bool wired = false;

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
        Vec3d projectInPlane(Vec3d v, Vec3d n, bool keep_length);
        Vec3d reflectInPlane(Vec3d v, Vec3d n);

        VRGeometryPtr makeTrianglePlane(int N, bool wire = false);

        void calcBezQuadPlane(bezVRPolygon<4>& q);
        void calcBezTrianglePlane(bezVRPolygon<3>& q);

    public:
        Patch();
        ~Patch();

        static PatchPtr create();

        //iteriert über die flächen der geometrie und macht bezierflächen hin
        VRObjectPtr fromGeometry(VRGeometryPtr geo, int N, bool wire = false);
};

OSG_END_NAMESPACE;

#endif // patch_H_INCLUDED
