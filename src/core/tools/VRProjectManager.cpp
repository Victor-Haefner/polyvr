#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>

using namespace OSG;
namespace fs = boost::filesystem;

VRProjectManager::VRProjectManager() : VRSprite("ProjectManager") { initSite(); }

VRProjectManagerPtr VRProjectManager::create() { return shared_ptr<VRProjectManager>(new VRProjectManager()); }

void VRProjectManager::initSite() {
    ;
}

void VRProjectManager::addItem(VRStoragePtr s) {
    if (s) vault.push_back(s);
}

void VRProjectManager::save(string path) {
    if (fs::exists(path)) path = fs::canonical(path).string();
    xmlpp::Document doc;
    xmlpp::Element* root = doc.create_root_node("Project", "", "VRP"); // name, ns_uri, ns_prefix
    for (auto v : vault) if (auto vs = v.lock()) vs->saveUnder(root);
    doc.write_to_file_formatted(path);
}

void VRProjectManager::load(string path) {
    if (fs::exists(path)) path = fs::canonical(path).string();

    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());
    xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(parser.get_document()->get_root_node());

    vault.clear();
    for (auto n : root->get_children()) {
        xmlpp::Element* e = dynamic_cast<xmlpp::Element*>(n);
        if (!e) continue;

        auto s = VRStorage::createFromStore(e);
        s->load(e);
        addItem(s);
    }
}
