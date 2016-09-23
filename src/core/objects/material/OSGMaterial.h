#ifndef OSGMATERIAL_H_INCLUDED
#define OSGMATERIAL_H_INCLUDED

#include <OpenSG/OSGMultiPassMaterial.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGMaterial {
    public:
        MultiPassMaterialMTRecPtr mat;

        OSGMaterial(MultiPassMaterialMTRecPtr mat = 0);
        static OSGMaterialPtr create(MultiPassMaterialMTRecPtr mat = 0);
};

OSG_END_NAMESPACE;

#endif // OSGMATERIAL_H_INCLUDED
