#include "VRObjectManager.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObjectManager::VRObjectManager() {}
VRObjectManager::~VRObjectManager() {}

VRObjectManagerPtr VRObjectManager::create() { return shared_ptr<VRObjectManager>(new VRObjectManager()); }

VRObjectPtr VRObjectManager::addObject(VRObjectPtr s) {
    addTemplate(s);
    auto o = templates[s.get()]->duplicate();
    instances.push_back(o);
    return o;
}

void VRObjectManager::addTemplate(VRObjectPtr s) {
    if (!templates.count(s.get())) templates[s.get()] = s;
}

OSG_END_NAMESPACE;
