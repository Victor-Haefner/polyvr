#ifndef MODULEBUILDINGS_H
#define MODULEBUILDINGS_H

#include "core/objects/object/VRObject.h"
#include "../VRWorldGeneratorFwd.h"
#include <map>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDistrict : public VRObject {
    private:
        VRGeometryPtr facades;
        VRGeometryPtr roofs;
        VRMaterialPtr b_mat;

        void init();

    public:
        VRDistrict();
        ~VRDistrict();

        static VRDistrictPtr create();

        void addBuilding( VRPolygon p );
};

OSG_END_NAMESPACE;

#endif // MODULEBUILDINGS_H

