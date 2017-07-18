#include "VRObjectManager.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRStorage_template.h"
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRObjectManager::Entry::Entry(string name) {
    setName("ObjectManagerEntry");
    store("pose", &pos);
    store("object", &type);
}

VRObjectManager::Entry::~Entry() {}

shared_ptr<VRObjectManager::Entry> VRObjectManager::Entry::create(string name) { return shared_ptr<Entry>(new Entry(name)); }

void VRObjectManager::Entry::set(posePtr p, string t) {
    pos = p;
    type = t;
}


VRObjectManager::VRObjectManager(string name) : VRObject(name) {
    storeMap("templates", &templatesByName, true);
    storeMap("instances", &entries, true);
    regStorageSetupFkt( VRFunction<int>::create("object manager setup", boost::bind(&VRObjectManager::setup, this)) );
}

VRObjectManager::~VRObjectManager() {}

VRObjectManagerPtr VRObjectManager::create(string name) { return shared_ptr<VRObjectManager>(new VRObjectManager(name)); }

void VRObjectManager::setup() {
    for (auto t : templatesByName) addTemplate(t.second);

    for (auto& t : entries) {
        if (!templatesByName.count(t.second->type)) { cout << "VRObjectManager::setup Warning, " << t.second->type << " is not a template!" << endl; continue; }
        auto o = copy(t.second->type, t.second->pos, false);
        o->show();
        o->setPose(t.second->pos);
    }
}

VRTransformPtr VRObjectManager::copy(string name, posePtr p, bool addToStore) {
    auto t = getTemplate(name);
    if (!t) return 0;
    auto dupe = dynamic_pointer_cast<VRTransform>( t->duplicate() );
    dupe->resetNameSpace();
    instances[dupe->getID()] = dupe;
    auto e = Entry::create();
    e->set(p,name);
    if (addToStore) entries[dupe->getName()] = e;
    addChild(dupe);
    dupe->setPose(p);
    dupe->setPersistency(0);
    dupe->addAttachment("asset",0);
    return dupe;
}

VRTransformPtr VRObjectManager::add(VRTransformPtr s) {
    if (!s) return 0;
    addTemplate(s, s->getBaseName());
    return copy(s->getBaseName(), s->getPose());
}

void VRObjectManager::addTemplate(VRTransformPtr s, string name) {
    if (!s) return;
    if (!templates.count(s.get())) {
        templates[s.get()] = s;
        s->setNameSpace("OMtemplate");
        if (name != "") {
            s->setName(name);
            templatesByName[name] = s;
        }
    }
}

VRTransformPtr VRObjectManager::getTemplate(string name) { return templatesByName.count(name) ? templatesByName[name] : 0; }

void VRObjectManager::rem(VRTransformPtr t) {
    if (!t) return;
    if (entries.count(t->getName())) entries.erase(t->getName());
    if (instances.count(t->getID())) instances.erase(t->getID());
    subChild(t);
}

void VRObjectManager::updateObject(VRTransformPtr t) {
    if (!t) return;
    if (entries.count(t->getName())) {
        entries[t->getName()]->pos = t->getPose();
    }
}

void VRObjectManager::clear() {
    entries.clear();
    instances.clear();
    templates.clear();
    templatesByName.clear();
    clearChildren();
}

vector<VRTransformPtr> VRObjectManager::getCatalog() {
    vector<VRTransformPtr> res;
    for (auto o : templates) res.push_back(o.second);
    return res;
}

OSG_END_NAMESPACE;
