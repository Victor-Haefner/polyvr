#ifndef OSGCORE_H_INCLUDED
#define OSGCORE_H_INCLUDED

#include <OpenSG/OSGNode.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGCore {
    public:
        NodeCoreMTRecPtr core;

        OSGCore(NodeCoreMTRecPtr core = 0);
        static OSGCorePtr create(NodeCoreMTRecPtr core = 0);
};

OSG_END_NAMESPACE;

#endif // OSGCORE_H_INCLUDED
