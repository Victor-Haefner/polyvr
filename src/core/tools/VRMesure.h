#ifndef VRMESURE_H_INCLUDED
#define VRMESURE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include "core/utils/VRFunctionFwd.h"
#include "VRAnalyticGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

class VRMeasure : public VRAnalyticGeometry {
    private:
        Vec3f p1,p2,p3;
        float d1,d2;
        float angle;

        void update();

    public:
        VRMeasure(string name);

        static VRMeasurePtr create(string name);
        VRMeasurePtr ptr();

        void setPoint(int i, Vec3f p);
        void rollPoints(Vec3f p);
};

OSG_END_NAMESPACE;

#endif // VRMESURE_H_INCLUDED
