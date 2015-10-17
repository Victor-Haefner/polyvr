#ifndef BASEWORLDOBJECT_H
#define BASEWORLDOBJECT_H

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class BaseWorldObject {
    public:
        VRGeometryPtr mesh;
};

OSG_END_NAMESPACE;

#endif // BASEWORLDOBJECT_H

