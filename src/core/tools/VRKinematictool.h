#ifndef VRKINEMATICTOOL_H_INCLUDED
#define VRKINEMATICTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include "core/math/pose.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/VRConstraint.h"
#include "VRAnalyticGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRJointTool : public VRAnalyticGeometry {
    private:
        VRTransformWeakPtr obj1;
        VRTransformWeakPtr obj2;
        pose anchor1;
        pose anchor2;
        bool lastAppended = true;

        void update();

    public:
        VRJointTool(string name);

        static VRJointToolPtr create(string name);
        VRJointToolPtr ptr();

        int append(VRTransformPtr t, pose p);
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRKINEMATICTOOL_H_INCLUDED
