#ifndef OSGTRANSFORM_H_INCLUDED
#define OSGTRANSFORM_H_INCLUDED

#include <OpenSG/OSGTransform.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGTransform {
    public:
        TransformMTRecPtr trans;

        OSGTransform(TransformMTRecPtr trans = 0);
        static OSGTransformPtr create(TransformMTRecPtr trans = 0);
};

OSG_END_NAMESPACE;

#endif // OSGTRANSFORM_H_INCLUDED
