#ifndef VRKINEMATICTOOL_H_INCLUDED
#define VRKINEMATICTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include <string>
#include "core/math/pose.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRJointTool : public VRGeometry {
    private:
        VRTransformWeakPtr obj1;
        VRTransformWeakPtr obj2;
        string obj1_name;
        string obj2_name;
        PosePtr anchor1;
        PosePtr anchor2;
        bool selected = true;
        bool active = true;
        bool lastAppended = true;
        VRAnalyticGeometryPtr ageo;

        VRUpdateCbPtr setupAfterCb;
        void delayed_setup();
        void updateConstraint();

    public:
        VRJointTool(string name);
        ~VRJointTool();

        static VRJointToolPtr create(string name = "None");
        VRJointToolPtr ptr();

        int append(VRTransformPtr t, PosePtr p);
        void setActive(bool b = true);
        void select(bool b = true);
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRKINEMATICTOOL_H_INCLUDED
