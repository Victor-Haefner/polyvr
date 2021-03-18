#include "VRProjectManager.h"

#include "core/utils/VRStorage.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/system/VRSystem.h"
#include <iostream>
#include <algorithm>

using namespace OSG;


VRProjectManager::VRProjectManager() : VRObject("ProjectManager") {
    storage.storeMap("settings", settings);
}

VRProjectManager::~VRProjectManager() {}

VRProjectManagerPtr VRProjectManager::create() { return shared_ptr<VRProjectManager>(new VRProjectManager()); }

void VRProjectManager::addItem(VRStoragePtr s, string mode) {
    if (!s) return;
    if (mode == "RELOAD") vault_reload.push_back(s);
    if (mode == "REBUILD") vault_rebuild.push_back(s);
    modesMap[s.get()] = mode;
    s->store("pmMode", &modesMap[s.get()]);
}

void VRProjectManager::remItem(VRStoragePtr s) {
    if (!s) return;
    vault_reload.erase(std::remove(vault_reload.begin(), vault_reload.end(), s), vault_reload.end());
    vault_rebuild.erase(std::remove(vault_rebuild.begin(), vault_rebuild.end(), s), vault_rebuild.end());
    if (modesMap.count(s.get())) modesMap.erase(s.get());
}

void VRProjectManager::setSetting(string s, string v) { settings[s] = v; }
string VRProjectManager::getSetting(string s, string d) { return settings.count(s) ? settings[s] : d; }

vector<VRStoragePtr> VRProjectManager::getItems() {
    vector<VRStoragePtr> res;
    for (auto v : vault_rebuild) if(v) res.push_back(v);
    return res;
}

void VRProjectManager::newProject(string path) {
    setName(path);
    vault_rebuild.clear();
    modesMap.clear();
}

void VRProjectManager::save(string path) {
    if (path == "") path = getName();
    if (exists(path)) path = canonical(path);
    cout << "VRProjectManager::save " << path << " (" << toString(vault_reload.size()) << " + " << toString(vault_rebuild.size()) << " objects)" << endl;

    XML xml;
    XMLElementPtr root = xml.newRoot("Project", "", ""); // name, ns_uri, ns_prefix
    storage.save(root);

    for (auto v : vault_reload) v->saveUnder(root, persistencyLvl);
    for (auto v : vault_rebuild) v->saveUnder(root, persistencyLvl);
    xml.write(path);
}

void VRProjectManager::load(string path) {
    if (path == "") path = getName();
    if (exists(path)) {
        setName(path);
        path = canonical(path);
    } else return;
    cout << "VRProjectManager::load " << path << endl;

    XML xml;
    xml.read(path, false);
    XMLElementPtr root = xml.getRoot();
    storage.load(root);

    vault_rebuild.clear();
    unsigned int i=0;
    for (auto e : root->getChildren()) {
        if (!e) continue;

        VRStoragePtr s;
        string mode = "RELOAD";
        if (e->hasAttribute("pmMode")) mode = e->getAttribute("pmMode");
        if (mode == "RELOAD") { if (i < vault_reload.size()) s = vault_reload[i]; i++; }
        if (mode == "REBUILD") { s = VRStorage::createFromStore(e); vault_rebuild.push_back(s); }
        if (!s) { cout << "VRProjectManager::load Warning! element unhandled"; continue; }

        auto ctx = VRStorageContext::create(mode == "RELOAD");
        s->load(e, ctx);
    }
}

void VRProjectManager::setPersistencyLevel(int p) { persistencyLvl = p; }


