#include "OSGObject.h"

using namespace OSG;

OSGObject::OSGObject(NodeMTRecPtr node) {
    this->node = node;
}

OSGObjectPtr OSGObject::create(NodeMTRecPtr node) { return shared_ptr<OSGObject>( new OSGObject(node) ); }
