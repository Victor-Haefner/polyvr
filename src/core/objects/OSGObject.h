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
        ~OSGObject();
        static OSGObjectPtr create(NodeMTRecPtr node = 0);

        bool hasAttachment(string name);
        void setAttachment(string name, string value);
        string getAttachment(string name);
};

OSG_END_NAMESPACE;

#endif // OSGOBJECT_H_INCLUDED
