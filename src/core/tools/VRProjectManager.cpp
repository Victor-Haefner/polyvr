#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>
#include <iostream>
#include <algorithm>

using namespace OSG;
using namespace std;
namespace fs = boost::filesystem;

VRProjectManager::VRProjectManager() : VRObject("ProjectManager") {}
VRProjectManager::~VRProjectManager() {}

VRProjectManagerPtr VRProjectManager::create() { return shared_ptr<VRProjectManager>(new VRProjectManager()); }

void VRProjectManager::addItem(VRStoragePtr s, string mode) {
    if (!s) return;
    if (mode == "RELOAD") vault_reload.push_back(s);
    if (mode == "REBUILD") vault_rebuild.push_back(s);
    s->store("pmMode", mode);
}

void VRProjectManager::remItem(VRStoragePtr s) {
    if (!s) return;
    vault_reload.erase(std::remove(vault_reload.begin(), vault_reload.end(), s), vault_reload.end());
    vault_rebuild.erase(std::remove(vault_rebuild.begin(), vault_rebuild.end(), s), vault_rebuild.end());
}

vector<VRStoragePtr> VRProjectManager::getItems() {
    vector<VRStoragePtr> res;
    for (auto v : vault_rebuild) if(v) res.push_back(v);
    return res;
}

void VRProjectManager::newProject(string path) {
    setName(path);
    vault_rebuild.clear();
}

void VRProjectManager::save(string path) {
    if (path == "") path = getName();
    if (fs::exists(path)) path = fs::canonical(path).string();
    cout << "VRProjectManager::save " << path << endl;

    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("Project", "", "VRP"); // name, ns_uri, ns_prefix
    for (auto v : vault_reload) {
        cout << "VRProjectManager::save " << v << " " << persistencyLvl << endl;
        v->saveUnder(root, persistencyLvl);
    }
    for (auto v : vault_rebuild) v->saveUnder(root, persistencyLvl);
    doc.write_to_file_formatted(path);
}

void VRProjectManager::load(string path) {
    if (path == "") path = getName();
    if (fs::exists(path)) {
        setName(path);
        path = fs::canonical(path).string();
    } else return;
    cout << "VRProjectManager::load " << path << endl;

    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());
    xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(parser.get_document()->get_root_node());

    vault_rebuild.clear();
    uint i=0;
    for (auto n : root->get_children()) {
        xmlpp::Element* e = dynamic_cast<xmlpp::Element*>(n);
        if (!e) continue;

        VRStoragePtr s;
        string mode = "RELOAD";
        if (e->get_attribute("pmMode")) mode = e->get_attribute("pmMode")->get_value();
        if (mode == "RELOAD") { if (i < vault_reload.size()) s = vault_reload[i]; i++; }
        if (mode == "REBUILD") { s = VRStorage::createFromStore(e); vault_rebuild.push_back(s); }
        if (!s) { cout << "VRProjectManager::load Warning! element unhandled"; continue; }

        s->load(e);
    }
}

void VRProjectManager::setPersistencyLevel(int p) { persistencyLvl = p; }


