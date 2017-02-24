#include "VRObjectGroupManager.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


void VRObjectGroupManager::addGroup(string group) {
    groups[group] = list<VRGroupPtr>();
}

void VRObjectGroupManager::updateObjects() {
    //cout << "VRObjectGroupManager updateObjects " << VRTransform::changedObjects.size() << " " << VRTransform::dynamicObjects.size() << endl;

    //update the Transform changelists
    for ( auto t : VRTransform::changedObjects ) {
        if (auto sp = t.lock()) sp->update();
    }
    VRTransform::changedObjects.clear();

    //update the dynamic objects
    for ( auto t : VRTransform::dynamicObjects ) {
        if (auto sp = t.lock()) sp->update();
        // TODO: else: remove the t from dynamicObjects
    }
}

VRObjectGroupManager::VRObjectGroupManager() {
    updateObjectsFkt = VRFunction<int>::create("ObjectManagerUpdate", boost::bind(&VRObjectGroupManager::updateObjects, this));
}

//GROUPS------------------------

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

OSG_END_NAMESPACE;
