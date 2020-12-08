#ifndef LAYER2D_H_INCLUDED
#define LAYER2D_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class Layer2D {
    public:
        struct Line {
            Line(Pnt2d p1, Pnt2d p2, Color3f c1, Color3f c2);

            Pnt2d p1;
            Pnt2d p2;
            Color3f c1;
            Color3f c2;
        };

    private:
        Vec2d projectVector(Pnt3d v, PosePtr plane);
        vector<Line> lines;

    public:
        Layer2D();
        ~Layer2D();

        vector<Line>& getLines();

        void project(VRObjectPtr obj, PosePtr plane);
        void projectGeometry(VRGeometryPtr geo, PosePtr plane, PosePtr transform);

        void slice(VRObjectPtr obj, PosePtr plane);
};

OSG_END_NAMESPACE;

#endif // LAYER2D_H_INCLUDED
