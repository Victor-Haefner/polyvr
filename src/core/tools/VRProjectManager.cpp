#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>

using namespace OSG;
namespace fs = boost::filesystem;

VRProjectManager::VRProjectManager() : VRObject("ProjectManager") {}

VRProjectManagerPtr VRProjectManager::create() { return shared_ptr<VRProjectManager>(new VRProjectManager()); }

void VRProjectManager::setPersistencies(int reload, int rebuild) { reload_persistency = reload; rebuild_persistency = rebuild; }
void VRProjectManager::addItem(VRStoragePtr s) { if (s) vault.push_back(s); }

void VRProjectManager::save(string path) {
    if (fs::exists(path)) path = fs::canonical(path).string();
    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("Project", "", "VRP"); // name, ns_uri, ns_prefix
    for (auto v : vault) {
        if (auto vs = v.lock()) {
            vs->saveUnder(root); // objects with a persistency above this value will be saved
        }
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

    vector<VRStorageWeakPtr> new_vault;

    int i=0;
    for (auto n : root->get_children()) {
        xmlpp::Element* e = dynamic_cast<xmlpp::Element*>(n);
        if (!e) continue;

        VRStoragePtr s;
        int p = VRStorage::getPersistency(e);
        cout << "VRProjectManager::load element persistency " << p << endl;

        if (p == rebuild_persistency) {
            cout << "VRProjectManager::load rebuild" << endl;
            s = VRStorage::createFromStore(e);
        }

        if (p == reload_persistency) {
            cout << "VRProjectManager::load reload" << endl;
            if (i < vault.size()) s = vault[i].lock();
        }


        if (!s) { cout << "VRProjectManager::load Warning! element unhandled"; continue; }
        s->load(e);
        new_vault.push_back(s);
        i++;
    }

    vault = new_vault;
}
