#include "VRName.h"
#include <vector>
#include <sstream>
#include <map>
#include <iostream>
#include "toString.h"
#include "VRStorage_template.h"
#include "VRFunction.h"
#include <libxml++/nodes/element.h>

using namespace std;

struct VRNamePool {
    string name;
    map<int, bool> names; // suffix, name
    map<int, bool> freed;
};

VRNameSpace::VRNameSpace(string nspace) : nspace(nspace) {}

int VRNameSpace::getSuffix(const string& base, const int& hint) {
    auto& pool = nameDict[base];
    if (hint != -1) if (!pool.names.count(hint)) return hint;
    int suffix = 0;
    if (nameDict.count(base)) {
        if (pool.freed.size() > 0) {
            suffix = pool.freed.begin()->first;
            pool.freed.erase(suffix);
        } else suffix = pool.names.size();
    }
    return suffix;
}

void VRNameSpace::removeName(const string& base, const int& suffix) {
    if (nameDict.count(base)) {
        nameDict[base].names.erase(suffix);
        nameDict[base].freed[suffix] = 0;
    }
}

string VRNameSpace::compileName(const string& base, const int& suffix) {
    if (!unique) return base;
    string name = base;
    if (suffix > 0) name += separator + toString(suffix);
    if (!nameDict.count(base)) nameDict[base] = VRNamePool();
    auto& pool = nameDict[base];
    pool.names[suffix] = true;
    if (pool.freed.count(suffix)) pool.freed.erase(suffix);
    return name;
}

void VRNameSpace::setSeparator(char   s) { separator = s; }
void VRNameSpace::setUniqueNames(bool b) { unique = b; }
void VRNameSpace::filterNameChars(string chars, char replacement) { filter = chars; filter_rep = replacement; }

void VRNameSpace::applyFilter(string& name) {
    for (char c : filter) replace(name.begin(), name.end(),c,filter_rep);
}

int VRNameSpace::getNameNumber() {
    int N = 0;
    for (auto n : nameDict) N += n.second.names.size();
    return N;
}

void VRNameSpace::print() {
    for (auto n : nameDict) {
        cout << "\n " << n.first << flush;
        for (auto s : n.second.names) cout << "\n  " << s.second << flush;
    }
}

class VRNameManager {
    private:
        map<string, VRNameSpace> nameDicts;

    public:
        VRNameManager() {}

        VRNameSpace* getNameSpace(string s) {
            if (!nameDicts.count(s)) nameDicts[s] = VRNameSpace(s);
            return &nameDicts[s];
        }

        void print() {
            for (auto& ns : nameDicts) ns.second.print();
        }
};

VRNameManager nmgr;
void VRName::printInternals() { nmgr.print(); }

VRName_base::VRName_base() {;}
VRName_base::~VRName_base() { setName(""); }

VRNameSpace* VRName_base::setNameSpace(string s) {
    nameSpaceName = s;
    if (nameSpace) {
        if(nameSpace->nspace == s) return nameSpace;
        nameSpace->removeName(base_name, name_suffix);
    }

    nameSpace = nmgr.getNameSpace(s);
    if (base_name != "") {
        string tmp = base_name;
        base_name = "";
        setName(tmp);
    }
    return nameSpace;
}

VRNameSpace* VRName_base::resetNameSpace() { return setNameSpace("__global__"); }

void VRName_base::compileName() {
    if (!nameSpace) setNameSpace(nameSpaceName);
    name = nameSpace->compileName(base_name, name_suffix);
}

string VRName_base::setName(string name) {
    if (!nameSpace) setNameSpace(nameSpaceName);
    nameSpace->applyFilter(name);
    if (base_name == name && name_suffix == 0) return this->name; // already named like that, return
    nameSpace->removeName(base_name, name_suffix); // check if already named, remove old name from dict
    if (name == "") return "";

    // check if passed name has a base|separator|suffix structure
    auto vs = splitString(name, nameSpace->separator);
    if (vs.size() > 1) {
        string last = *vs.rbegin();
        bool isNumber = (last.find_first_not_of( "0123456789" ) == string::npos);
        if (isNumber) {
            base_name = vs[0];
            for (uint i=1; i<vs.size()-1; i++) base_name += nameSpace->separator + vs[i];
            name_suffix = nameSpace->getSuffix(base_name, toInt(last));
            compileName();
            return this->name;
        }
    }

    base_name = name; // set new base name
    if (nameSpace->unique) name_suffix = nameSpace->getSuffix(base_name);
    compileName();
    return this->name;
}

string VRName_base::getName() { return name; }
string VRName_base::getBaseName() { return base_name; }
int VRName_base::getNameSuffix() { return name_suffix; }

/*int VRName_base::getBaseNameNumber() { return nmgr.getBaseNameNumber(); }
int VRName_base::getNameNumber() { return nmgr.getNameNumber(); }
void VRName_base::printNameDict() { nmgr.print(); }*/

VRName::VRName() {
    store("name_suffix", &name_suffix);
    store("base_name", &base_name);
    store("name_space", &nameSpaceName);

    regStorageSetupFkt( VRUpdateCb::create("name_update", boost::bind(&VRName_base::compileName, dynamic_cast<VRName_base*>(this))) );
}

VRName::~VRName() {}
