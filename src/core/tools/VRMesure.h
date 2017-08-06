#ifndef VRMESURE_H_INCLUDED
#define VRMESURE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include "core/utils/VRFunctionFwd.h"
#include "core/math/pose.h"
#include "VRAnalyticGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

class VRMeasure : public VRAnalyticGeometry {
    private:
        pose P1,P2,P3;

        void update();

    public:
        VRMeasure(string name);

        static VRMeasurePtr create(string name = "measure");
        VRMeasurePtr ptr();

        void setPoint(int i, posePtr p);
        void rollPoints(posePtr p);
};

OSG_END_NAMESPACE;

#endif // VRMESURE_H_INCLUDED
