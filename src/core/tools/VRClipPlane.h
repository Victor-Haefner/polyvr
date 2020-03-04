#ifndef VRCLIPPLANE_H_INCLUDED
#define VRCLIPPLANE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"

#include <list>

#include "core/objects/geometry/VRGeometry.h"
#include "core/tools/VRToolsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMaterial;

class VRClipPlane : public VRGeometry {
    private:
        vector<VRMaterialPtr> mats;
        VRObjectPtr tree = 0;
        bool active = false;

        Vec4d getEquation();
        void activate();
        void deactivate();

    public:
        VRClipPlane(string name);
        ~VRClipPlane();

        static VRClipPlanePtr create(string name = "clipPlane");
        VRClipPlanePtr ptr();

        bool isActive();
        void setActive(bool a);
        void setTree(VRObjectPtr obj);
        void setSize(float W, float H);
};

OSG_END_NAMESPACE;

#endif // VRCLIPPLANE_H_INCLUDED
