#ifndef VROBJECTMANAGER_H_INCLUDED
#define VROBJECTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include <string>
#include <list>
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGroup;
class VRObject;

class VRObjectManager {
    protected:
        map<string, list<VRGroup*>* > groups;
        shared_ptr<VRFunction<int> > updateObjectsFkt;

        void addGroup(string group);

        void updateObjects();

    public:
        VRObjectManager();

        //GROUPS------------------------

        void addToGroup(VRGroup* obj, string group);

        list<VRGroup*>* getGroup(string group);

        vector<string> getGroupList();
};

OSG_END_NAMESPACE;

#endif // VROBJECTMANAGER_H_INCLUDED
