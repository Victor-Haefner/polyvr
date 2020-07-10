#ifndef VRANALYTICGEOMETRY_H_INCLUDED
#define VRANALYTICGEOMETRY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>
#include <string>

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/VRTransform.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAnalyticGeometry : public VRTransform {
    private:
        VRAnnotationEnginePtr ae = 0;
        VRGeometryPtr vectorLinesGeometry = 0;
        VRGeometryPtr vectorEndsGeometry = 0;
        VRGeometryPtr circlesGeometry = 0;

        VRMaterialPtr vecMat = 0;
        VRMaterialPtr pntMat = 0;
        VRMaterialPtr cirMat = 0;

        Color4f foreground;
        Color4f background;
        Vec3d labelOffset;

        static string circle_vp;
        static string circle_fp;

        void resize(int i, int j = -1, int k = -1);

    public:
        VRAnalyticGeometry(string name);
        ~VRAnalyticGeometry();

        static VRAnalyticGeometryPtr create(string name = "AnalyticGeometry");
        VRAnalyticGeometryPtr ptr();
        void init();

        //int getNewID(); //TODO
        //void remove(int ID);

        void setLabelParams(float size, bool screen_size = false, bool billboard = false, Color4f fg = Color4f(0,0,0,1), Color4f bg = Color4f(0,0,0,0), Vec3d offset = Vec3d(0,0,0));
        VRAnnotationEnginePtr getAnnotationEngine();

        void setVector(int i, Vec3d pos, Vec3d vec, Color3f color, string label="", bool doArrow = false);
        void setAngle(int i, Vec3d pos, Vec3d v1, Vec3d v2, Color3f c1, Color3f c2, string label="");
        void setCircle(int i, Vec3d pos, Vec3d norm, float r, Color3f color, string label="");

        int addVector(Vec3d pos, Vec3d vec, Color3f color, string label="", bool doArrow = false);

        void clear();

        void setZOffset(float factor, float bias);
};

OSG_END_NAMESPACE;

#endif // VRANALYTICGEOMETRY_H_INCLUDED
