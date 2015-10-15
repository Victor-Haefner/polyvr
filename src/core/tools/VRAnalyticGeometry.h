#ifndef VRANALYTICGEOMETRY_H_INCLUDED
#define VRANALYTICGEOMETRY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>

#include "core/objects/VRObjectFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAnalyticGeometry : public VRObject {
    public:
        struct Element {
            string type;
            string label;
            Vec3f color;
        };

        struct Vector : Element {
            Vec3f v;
            Vec3f p;
        };

    private:
        VRAnnotationEnginePtr ae = 0;
        VRGeometryPtr vectorLinesGeometry = 0;
        VRGeometryPtr vectorEndsGeometry = 0;
        vector<Vector> vectors;

    public:
        VRAnalyticGeometry();
        ~VRAnalyticGeometry();

        static VRAnalyticGeometryPtr create();
        VRAnalyticGeometryPtr ptr();

        void setLabelSize(float s);

        void setVector(int i, Vec3f pos, Vec3f vec, Vec3f color, string label="");
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRANALYTICGEOMETRY_H_INCLUDED
