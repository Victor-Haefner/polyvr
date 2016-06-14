#include "OSGCore.h"

using namespace OSG;

OSGCore::OSGCore(NodeCoreMTRecPtr core) {
    this->core = core;
}

OSGCorePtr OSGCore::create(NodeCoreMTRecPtr core) { return shared_ptr<OSGCore>( new OSGCore(core) ); }
