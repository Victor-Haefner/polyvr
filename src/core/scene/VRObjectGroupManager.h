#ifndef VROBJECTGROUPMANAGER_H_INCLUDED
#define VROBJECTGROUPMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include <string>
#include <list>
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObjectGroupManager {
    protected:
        map<string, list<VRGroupPtr> > groups;
        void addGroup(string group);

    public:
        VRObjectGroupManager();

        void addToGroup(VRGroupPtr obj, string group);
        list<VRGroupPtr> getGroup(string group);
        vector<string> getGroupList();
};

OSG_END_NAMESPACE;

#endif // VROBJECTGROUPMANAGER_H_INCLUDED
