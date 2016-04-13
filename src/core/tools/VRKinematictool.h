#ifndef VRKINEMATICTOOL_H_INCLUDED
#define VRKINEMATICTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include "core/math/pose.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRJointTool : public VRGeometry {
    private:
        VRTransformWeakPtr obj1;
        VRTransformWeakPtr obj2;
        string obj1_name;
        string obj2_name;
        pose anchor1;
        pose anchor2;
        bool selected = true;
        bool active = true;
        bool lastAppended = true;
        VRAnalyticGeometryPtr ageo;

        void delayed_setup();
        void setup();
        void updateVis();

    public:
        VRJointTool(string name);
        ~VRJointTool();

        static VRJointToolPtr create(string name = "None");
        VRJointToolPtr ptr();

        int append(VRTransformPtr t, pose p);
        void setActive(bool b = true);
        void select(bool b = true);
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRKINEMATICTOOL_H_INCLUDED
