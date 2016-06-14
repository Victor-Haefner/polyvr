#ifndef OSGOBJECT_H_INCLUDED
#define OSGOBJECT_H_INCLUDED

#include <OpenSG/OSGNode.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGObject {
    public:
        NodeMTRecPtr node;
        map<VRObject*, NodeMTRecPtr> links;

        OSGObject(NodeMTRecPtr node = 0);
        static OSGObjectPtr create(NodeMTRecPtr node = 0);
};

OSG_END_NAMESPACE;

#endif // OSGOBJECT_H_INCLUDED
