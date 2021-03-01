#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/xml.h"

using namespace OSG;


VRStorageContextPtr VRStorageContext::create(bool onlyReload) {
    auto c = VRStorageContextPtr( new VRStorageContext() );
    c->onlyReload = onlyReload;
    return c;
}

map<string, VRStorageFactoryCbPtr> VRStorage::factory = map<string, VRStorageFactoryCbPtr>();

VRStorage::VRStorage() {
    store("persistency", &persistency);
}

VRStorage::~VRStorage() {
    //cout << "~VRStorage" << endl;
}

string VRStorage::getDescription() {
    string d = "[";
    bool first = true;
    for (auto s : storage) {
        if (!first) d += ", ";
        first = false;
        d += "\""+s.first+"\"";
    }
    return d+"]";
}

void VRStorage::setPersistency(int p) { persistency = p; }
int VRStorage::getPersistency() { return persistency; }
void VRStorage::regStorageSetupBeforeFkt(VRStorageCbPtr u) { f_setup_before.push_back(u); }
void VRStorage::regStorageSetupFkt(VRStorageCbPtr u) { f_setup.push_back(u); }
void VRStorage::regStorageSetupAfterFkt(VRUpdateCbPtr u) { f_setup_after.push_back(u); }
void VRStorage::setStorageType(string t) { type = t; }

void VRStorage::load_strstr_map_cb(map<string, string>* m, string tag, VRStorageCbParams p) {
    p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        string name = el->getName();
        (*m)[name] = el->getText();
    }
}

void VRStorage::save_strstr_map_cb(map<string, string>* m, string tag, VRStorageCbParams p) {
    if (m->size() == 0) return;
    p.e = p.e->addChild(tag);
    for (auto t : *m) {
        auto e2 = p.e->addChild(t.first);
        e2->setText(t.second);
    }
}

void VRStorage::storeMap(string tag, map<string, string>& m) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_strstr_map_cb, this, &m, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_strstr_map_cb, this, &m, tag, placeholders::_1 ) );
    storage[tag] = b;
}

void VRStorage::setOverrideCallbacks(bool b) { overrideCallbacks = b; }

void VRStorage::save(XMLElementPtr e, int p) {
    if (e == 0) return;
    if (persistency <= p) return;
    VRStorageCbParams sp = {e, p};
    for (auto s : storage) (*s.second.f2)(sp);
}

XMLElementPtr VRStorage::saveUnder(XMLElementPtr e, int p, string t) {
    string tag = type;
    if (t != "") tag = t;
    if (e == 0) return 0;
    //cout << "saveUnder " << t << " (" << p << "," << persistency << ") " << (persistency <= p) << " " << getDescription() << endl;
    if (persistency <= p) return 0;
    e = e->addChild(tag);
    save(e, p);
    return e;
}

void VRStorage::load(XMLElementPtr e, VRStorageContextPtr context) {
    if (e == 0) return;
    for (auto f : f_setup_before) (*f)(context);
    for (auto s : storage) (*s.second.f1)({e,0});
    for (auto f : f_setup) (*f)(context);
    for (auto f : f_setup_after) VRSceneManager::get()->queueJob(f, 0, 0, false);
    f_setup_after.clear();
}

XMLElementPtr VRStorage::loadChildFrom(XMLElementPtr e, string t, VRStorageContextPtr context) {
    string tag = type;
    if (t != "") tag = t;
    e = e->getChild(tag);
    load( e, context );
    return e;
}

XMLElementPtr VRStorage::loadChildIFrom(XMLElementPtr e, int i, VRStorageContextPtr context) {
    e = e->getChild(i);
    load( e, context );
    return e;
}

int VRStorage::getPersistency(XMLElementPtr e) {
    if (!e->hasAttribute("persistency")) return 0;
    return toInt( e->getAttribute("persistency") );
}

VRStoragePtr VRStorage::createFromStore(XMLElementPtr e, bool verbose) {
    if (!e->hasAttribute("type")) {
        if (verbose) cout << "VRStorage::createFromStore WARNING: element " << e->getName() << " has no attribute type\n";
        return 0;
    }

    string type = e->getAttribute("type");
    //cout << "VRStorage::createFromStore " << type << " " << factory.count(type) << endl;
    if (!factory.count(type)) {
        if (verbose) cout << "VRStorage::createFromStore WARNING: factory can not handle type " << type << endl;
        return 0;
    }

    VRStoragePtr res;
    (*factory[type])(res);
    return res;
}

namespace OSG {
int getID(XMLElementPtr el) {
    if (!el->hasAttribute("ID")) return -1;
    string _ID = el->getAttribute("ID");
    int ID;
    toValue( _ID, ID );
    return ID;
}
}

bool VRStorage::saveToFile(string path, bool createDirs) {
    XML xml;
    XMLElementPtr root = xml.newRoot("ProjectsList", "", ""); // name, ns_uri, ns_prefix
    save(root);
    xml.write(path);
    return true;
}

bool VRStorage::loadFromFile(string path, VRStorageContextPtr context) {
    if (exists(path)) path = canonical(path);
    else return false;

    XML xml;
    xml.read(path, false);
    load(xml.getRoot(), context);
    return true;
}

