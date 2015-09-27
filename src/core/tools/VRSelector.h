#ifndef VRSELECTOR_H_INCLUDED
#define VRSELECTOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMaterial.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelector {
    private:
        Vec3f color;

        map<VRGeometryPtr, VRMaterialPtr> orig_mats;
        VRObjectPtr selection = 0;

        VRMaterialPtr getMat();

        void deselect();

    public:
        VRSelector();

        void select(VRObjectPtr obj);
        VRObjectPtr get();

        void setColor(Vec3f c);
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED
