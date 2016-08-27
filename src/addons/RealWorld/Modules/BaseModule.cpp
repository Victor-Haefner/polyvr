#include "BaseModule.h"
#include "core/objects/object/VRObject.h"

using namespace OSG;

BaseModule::BaseModule(string name, bool t, bool p) {
    this->name = name;
    doPhysicalize = p;
    useThreads = t;
    root = VRObject::create(name+"Root");
    root->setPersistency(0);
}

VRObjectPtr BaseModule::getRoot() { return root; }
string BaseModule::getName() { return name; }


