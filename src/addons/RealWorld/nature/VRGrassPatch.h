#ifndef VRGRASSPATCH_H_INCLUDED
#define VRGRASSPATCH_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"
#include "addons/RealWorld/VRRealWorldFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeoData;

class VRGrassPatch : public VRObject {
    private:
        VRMaterialPtr mat;

        void addGrassBlade(VRGeoData& data);

    public:
        VRGrassPatch();
        ~VRGrassPatch();
        static VRGrassPatchPtr create();
};

OSG_END_NAMESPACE;

#endif // VRGRASSPATCH_H_INCLUDED
