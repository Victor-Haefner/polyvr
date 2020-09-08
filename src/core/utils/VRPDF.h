#ifndef VRPDF_H_INCLUDED
#define VRPDF_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <string>

struct _cairo_surface;
struct _cairo;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDF {
    private:
        _cairo_surface* surface = 0;
        _cairo* cr = 0;

        int res = 300;
        float W = 100;
        float H = 100;
        float Ox = 100;
        float Oy = 100;

        Vec2d projectVector(Pnt3d v, PosePtr plane);

    public:
        VRPDF(string path);
        ~VRPDF();

        static VRPDFPtr create(string path);

        void write(string path);

        void drawLine(Vec2d p1, Vec2d p2, Color3f c1, Color3f c2);
        void drawText();

        void project(VRObjectPtr obj, PosePtr plane);
        void projectGeometry(VRGeometryPtr geo, PosePtr plane, PosePtr transform);

        void slice(VRObjectPtr obj, PosePtr plane);
};

OSG_END_NAMESPACE;

#endif // VRPDF_H_INCLUDED
