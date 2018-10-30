#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/system/VRSystem.h"
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

template<> string typeName(const OSG::VRStoragePtr& o) { return "Storage"; }

OSG_BEGIN_NAMESPACE;
using namespace std;

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

void VRStorage::load_strstr_map_cb(map<string, string>* m, string tag, xmlpp::Element* e) {
    e = getChild(e, tag);
    if (!e) return;
    for (auto el : getChildren(e)) {
        string name = el->get_name();
        string val = el->get_child_text()->get_content();
        (*m)[name] = val;
    }
}

void VRStorage::save_strstr_map_cb(map<string, string>* m, string tag, xmlpp::Element* e) {
    if (m->size() == 0) return;
    e = e->add_child(tag);
    for (auto t : *m) {
        auto e2 = e->add_child(t.first);
        e2->set_child_text(t.second);
    }
}

void VRStorage::storeMap(string tag, map<string, string>& m) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_strstr_map_cb, this, &m, tag, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_strstr_map_cb, this, &m, tag, _1 ) );
    storage[tag] = b;
}

void VRStorage::setOverrideCallbacks(bool b) { overrideCallbacks = b; }

void VRStorage::save(xmlpp::Element* e, int p) {
    if (e == 0) return;
    if (persistency <= p) return;
    for (auto s : storage) (*s.second.f2)(e);
}

xmlpp::Element* VRStorage::saveUnder(xmlpp::Element* e, int p, string t) {
    string tag = type;
    if (t != "") tag = t;
    if (e == 0) return 0;
    //cout << "saveUnder " << t << " (" << p << "," << persistency << ") " << (persistency <= p) << " " << getDescription() << endl;
    if (persistency <= p) return 0;
    e = e->add_child(tag);
    save(e, p);
    return e;
}

void VRStorage::load(xmlpp::Element* e, VRStorageContextPtr context) {
    if (e == 0) return;
    for (auto f : f_setup_before) (*f)(context);
    for (auto s : storage) (*s.second.f1)(e);
    for (auto f : f_setup) (*f)(context);
    for (auto f : f_setup_after) VRSceneManager::get()->queueJob(f);
    f_setup_after.clear();
}

void VRStorage::loadChildFrom(xmlpp::Element* e, string t, VRStorageContextPtr context) {
    string tag = type;
    if (t != "") tag = t;
    load( getChild(e, tag), context );
}

int VRStorage::getPersistency(xmlpp::Element* e) {
    if (!e->get_attribute("persistency")) return 0;
    return toInt( e->get_attribute("persistency")->get_value() );
}

VRStoragePtr VRStorage::createFromStore(xmlpp::Element* e, bool verbose) {
    if (!e->get_attribute("type")) {
        if (verbose) cout << "VRStorage::createFromStore WARNING: element " << e->get_name() << " has no attribute type\n";
        return 0;
    }

    string type = e->get_attribute("type")->get_value();
    //cout << "VRStorage::createFromStore " << type << " " << factory.count(type) << endl;
    if (!factory.count(type)) {
        if (verbose) cout << "VRStorage::createFromStore WARNING: factory can not handle type " << type << endl;
        return 0;
    }

    VRStoragePtr res;
    (*factory[type])(res);
    return res;
}

xmlpp::Element* VRStorage::getChild(xmlpp::Element* e, string c) {
    if (e == 0) return 0;
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        if (el->get_name() == c) return el;
    }
    return 0;
}

vector<xmlpp::Element*> VRStorage::getChildren(xmlpp::Element* e) {
    vector<xmlpp::Element*> res;
    if (!e) return res;
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        res.push_back(el);
    }
    return res;
}

int getID(xmlpp::Element* el) {
    if (!el->get_attribute("ID")) return -1;
    string _ID = el->get_attribute("ID")->get_value();
    int ID;
    toValue( _ID, ID );
    return ID;
}

bool VRStorage::saveToFile(string path, bool createDirs) {
    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("ProjectsList", "", "VRP"); // name, ns_uri, ns_prefix
    save(root);
    doc.write_to_file_formatted(path);
    return true;
}

bool VRStorage::loadFromFile(string path, VRStorageContextPtr context) {
    if (exists(path)) path = canonical(path);
    else return false;

    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());
    xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(parser.get_document()->get_root_node());
    load(root, context);

    /*if (root == 0) return true;
    cout << "VRStorage::loadFromFile " << path << endl;
    //for (auto f : f_setup_before) (*f)();
    for (auto s : storage) (*s.second.f1)(root);
    for (auto f : f_setup) (*f)();
    //for (auto f : f_setup_after) VRSceneManager::get()->queueJob(f);
    //f_setup_after.clear();*/

    return true;
}

OSG_END_NAMESPACE;
