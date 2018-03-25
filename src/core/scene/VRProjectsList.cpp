#include "VRProjectsList.h"
#include "core/utils/VRStorage_template.h"

using namespace OSG;

VRProjectEntry::VRProjectEntry(string p, string t) {
    set(p,t);

    store("path", &path);
    store("timestamp", &timestamp);
}

VRProjectEntry::~VRProjectEntry() {}
VRProjectEntryPtr VRProjectEntry::create(string p, string t) { return VRProjectEntryPtr( new VRProjectEntry(p,t) ); }
VRProjectEntryPtr VRProjectEntry::create(string p) { return VRProjectEntryPtr( new VRProjectEntry(p,"") ); }

void VRProjectEntry::set(string p, string t) {
    path = p;
    timestamp = t;
}

string VRProjectEntry::getName() { return path; } // needed for storage map key
string VRProjectEntry::getPath() { return path; }
string VRProjectEntry::getTimestamp() { return timestamp; }



VRProjectsList::VRProjectsList() {
    storeMap("entries", &entries, true);
}

VRProjectsList::~VRProjectsList() {}
VRProjectsListPtr VRProjectsList::create() { return VRProjectsListPtr( new VRProjectsList() ); }

int VRProjectsList::size() { return entries.size(); }

void VRProjectsList::addEntry(VRProjectEntryPtr e) { entries[e->getPath()] = e; }
bool VRProjectsList::hasEntry(string path) { return entries.count(path) != 0; }
void VRProjectsList::remEntry(string path) { if (hasEntry(path)) entries.erase(path); }
void VRProjectsList::clear() { entries.clear(); }
map<string, VRProjectEntryPtr> VRProjectsList::getEntries() { return entries; }

vector<string> VRProjectsList::getPaths() {
    vector<string> res;
    for (auto e : entries) res.push_back(e.first);
    return res;
}

