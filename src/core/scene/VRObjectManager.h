#ifndef VROBJECTMANAGER_H_INCLUDED
#define VROBJECTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObjectManager {
    private:
        map<VRObject*, VRObjectPtr> templates;
        vector<VRObjectWeakPtr> instances;

    public:
        VRObjectManager();
        ~VRObjectManager();
        static VRObjectManagerPtr create();

        void addTemplate(VRObjectPtr s); // add object, returns duplicate, first time the object is stored as template
        VRObjectPtr addObject(VRObjectPtr s); // add object, returns duplicate, first time the object is stored as template
};

OSG_END_NAMESPACE;

#endif // VROBJECTMANAGER_H_INCLUDED
