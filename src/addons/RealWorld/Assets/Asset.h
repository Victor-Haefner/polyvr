#ifndef ASSET_H_INCLUDED
#define ASSET_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Asset {
    public:
        static VRGeometryPtr merge(VRObjectPtr obj);
};

OSG_END_NAMESPACE;

#endif // ASSET_H_INCLUDED
