#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>

using namespace OSG;
namespace fs = boost::filesystem;

VRProjectManager::VRProjectManager() : VRObject("ProjectManager") {}

VRProjectManagerPtr VRProjectManager::create() { return shared_ptr<VRProjectManager>(new VRProjectManager()); }

void VRProjectManager::addItem(VRStoragePtr s, string mode) {
    if (!s) return;
    if (mode == "RELOAD") vault_reload.push_back(s);
    if (mode == "REBUILD") vault_rebuild.push_back(s);
}

vector<VRStoragePtr> VRProjectManager::getItems() {
    vector<VRStoragePtr> res;
    for (auto v : vault_rebuild) if(v) res.push_back(v);
    return res;
}

void VRProjectManager::save(string path) {
    if (fs::exists(path)) path = fs::canonical(path).string();
    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("Project", "", "VRP"); // name, ns_uri, ns_prefix

    for (auto v : vault_reload) {
        int p = v->getPersistency();
        v->setPersistency(1);
        v->saveUnder(root);
        v->setPersistency(p);
    }

    for (auto v : vault_rebuild) {
        int p = v->getPersistency();
        v->setPersistency(2);
        v->saveUnder(root);
        v->setPersistency(p);
    }

    doc.write_to_file_formatted(path);
}

void VRProjectManager::load(string path) {
    cout << "VRProjectManager::load " << path << endl;
    if (fs::exists(path)) path = fs::canonical(path).string();

    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());
    xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(parser.get_document()->get_root_node());

    vault_rebuild.clear();
    int i=0;
    for (auto n : root->get_children()) {
        xmlpp::Element* e = dynamic_cast<xmlpp::Element*>(n);
        if (!e) continue;

        VRStoragePtr s;
        int p = VRStorage::getPersistency(e);
        if (p == 1) { if (i < vault_reload.size()) s = vault_reload[i]; i++; }
        if (p == 2) { s = VRStorage::createFromStore(e); vault_rebuild.push_back(s); }
        if (!s) { cout << "VRProjectManager::load Warning! element unhandled"; continue; }

        s->load(e);
    }
}
