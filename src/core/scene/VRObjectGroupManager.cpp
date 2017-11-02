#include "VRObjectGroupManager.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"

using namespace OSG;
using namespace std;

VRObjectGroupManager::VRObjectGroupManager() {}

void VRObjectGroupManager::addGroup(string group) {
    groups[group] = list<VRGroupPtr>();
}

void VRObjectGroupManager::addToGroup(VRGroupPtr obj, string group) {
    if (!groups.count(group)) addGroup(group);
    groups[group].push_back(obj);
    obj->setGroup(group);
}

list<VRGroupPtr> VRObjectGroupManager::getGroup(string group) {
    return groups.count(group) ? groups[group] : list<VRGroupPtr>();
}

vector<string> VRObjectGroupManager::getGroupList() {
    vector<string> grps;
    for (auto g : groups) grps.push_back(g.first);
    return grps;
}
