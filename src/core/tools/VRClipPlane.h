#ifndef VRCLIPPLANE_H_INCLUDED
#define VRCLIPPLANE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include <list>

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMaterial;

class VRClipPlane : public VRGeometry {
    private:
        vector<VRMaterialPtr> mats;
        VRObjectPtr tree = 0;
        bool active = false;

        Vec4f getEquation();
        void activate();
        void deactivate();

    public:
        VRClipPlane(string name);

        static VRClipPlanePtr create(string name);
        VRClipPlanePtr ptr();

        bool isActive();
        void setActive(bool a);
        void setTree(VRObjectPtr obj);
};

OSG_END_NAMESPACE;

#endif // VRCLIPPLANE_H_INCLUDED
