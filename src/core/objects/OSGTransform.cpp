#include "OSGTransform.h"

using namespace OSG;

OSGTransform::OSGTransform(TransformMTRecPtr trans) {
    this->trans = trans;
}

OSGTransformPtr OSGTransform::create(TransformMTRecPtr trans) { return OSGTransformPtr( new OSGTransform(trans) ); }
