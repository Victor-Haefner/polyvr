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
    map<int, string> names; // suffix, name
    map<int, bool> freed;
};

struct VRNameSpace {
    string nspace;
    map<string, VRNamePool> nameDict; // key is base name

    VRNameSpace(string nspace = "") : nspace(nspace) {}

    int getSuffix(const string& base, const int& hint) {
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

    void removeName(const string& base, const int& suffix) {
        if (nameDict.count(base)) {
            nameDict[base].names.erase(suffix);
            nameDict[base].freed[suffix] = 0;
        }
    }

    string compileName(const string& base, const int& suffix, const char separator) {
        string name = base;
        if (suffix > 0) name += separator + toString(suffix);
        if (!nameDict.count(base)) nameDict[base] = VRNamePool();
        auto& pool = nameDict[base];
        pool.names[suffix] = name;
        if (pool.freed.count(suffix)) pool.freed.erase(suffix);
        return name;
    }

    int getNameNumber() {
        int N = 0;
        for (auto n : nameDict) N += n.second.names.size();
        return N;
    }

    void print() {
        for (auto n : nameDict) {
            cout << "\n " << n.first << flush;
            for (auto s : n.second.names) cout << "\n  " << s.second << flush;
        }
    }
};

class VRNameManager {
    private:
        map<string, VRNameSpace> nameDicts;

    public:
        VRNameManager() {}

        int getSuffix(const string& nspace, const string& base, const int& hint = -1) {
            if (!nameDicts.count(nspace)) nameDicts[nspace] = VRNameSpace(nspace);
            return nameDicts[nspace].getSuffix(base, hint);
        }

        void removeName(const string& nspace, const string& base, const int& suffix) {
            if (!nameDicts.count(nspace)) return;
            nameDicts[nspace].removeName(base, suffix);
        }

        string compileName(const string& nspace, const string& base, const int& suffix, const char separator) {
            if (!nameDicts.count(nspace)) nameDicts[nspace] = VRNameSpace(nspace);
            return nameDicts[nspace].compileName(base, suffix, separator);
        }

        int getBaseNameNumber() {
            int N = 0;
            for (auto ns : nameDicts) N += ns.second.nameDict.size();
            return N;
        }

        int getNameNumber() {
            int N = 0;
            for (auto ns : nameDicts) N += ns.second.getNameNumber();
            return N;
        }

        void print() {
            for (auto ns : nameDicts) {
                cout << "\n" << ns.first << flush;
                ns.second.print();
            }
        }
};

VRNameManager nmgr;

VRName_base::VRName_base() {;}
VRName_base::~VRName_base() { setName(""); }

void VRName_base::setSeparator(char   s) { separator = s; }
void VRName_base::setNameSpace(string s) {
    if (nameSpace == s) return;
    nmgr.removeName(nameSpace, base_name, name_suffix);
    nameSpace = s;
    if (base_name != "") {
        string tmp = base_name;
        base_name = "";
        setName(tmp);
    }
}

void VRName_base::resetNameSpace() { setNameSpace("__global__"); }

void VRName_base::compileName() {
    setNameSpace(nameSpace);
    name = base_name;
    if (unique) name = nmgr.compileName(nameSpace, base_name, name_suffix, separator);
}

string VRName_base::setName(string name) {
    for (char c : filter) replace(name.begin(), name.end(),c,filter_rep);
    if (base_name == name && name_suffix == 0) return this->name; // already named like that, return
    nmgr.removeName(nameSpace, base_name, name_suffix); // check if already named, remove old name from dict
    if (name == "") return "";

    // check if passed name has a base|separator|suffix structure
    auto vs = splitString(name, separator);
    if (vs.size() > 1) {
        string last = *vs.rbegin();
        bool isNumber = (last.find_first_not_of( "0123456789" ) == string::npos);
        if (isNumber) {
            base_name = vs[0];
            for (uint i=1; i<vs.size()-1; i++) base_name += separator + vs[i];
            name_suffix = nmgr.getSuffix(nameSpace, base_name, toInt(last));
            compileName();
            return this->name;
        }
    }

    base_name = name; // set new base name
    if (unique) name_suffix = nmgr.getSuffix(nameSpace, base_name);
    compileName();
    return this->name;
}
string VRName_base::getName() { return name; }
string VRName_base::getBaseName() { return base_name; }
int VRName_base::getNameSuffix() { return name_suffix; }
void VRName_base::setUniqueName(bool b) { unique = b; }
void VRName_base::filterNameChars(string chars, char replacement) { filter = chars; filter_rep = replacement; }

void VRName_base::saveName(xmlpp::Element* e) {
    e->set_attribute("name_suffix", toString(name_suffix));
    e->set_attribute("base_name", base_name);
    e->set_attribute("name_space", nameSpace);
}

void VRName_base::loadName(xmlpp::Element* e) {
    //cout << "\nLOAD Name " << this;
    nmgr.removeName(nameSpace, base_name, name_suffix); // check if allready named, remove old name from dict

    if (e->get_attribute("name_suffix")) name_suffix = toInt(e->get_attribute("name_suffix")->get_value());
    if (e->get_attribute("base_name")) base_name = e->get_attribute("base_name")->get_value();
    if (e->get_attribute("name_space")) nameSpace = e->get_attribute("name_space")->get_value();
    compileName();
}

int VRName_base::getBaseNameNumber() { return nmgr.getBaseNameNumber(); }
int VRName_base::getNameNumber() { return nmgr.getNameNumber(); }
void VRName_base::printNameDict() { nmgr.print(); }

VRName::VRName() {
    store("name_suffix", &name_suffix);
    store("base_name", &base_name);
    store("name_space", &nameSpace);
    store("unique", &unique);

    regStorageSetupFkt( VRUpdateCb::create("name_update", boost::bind(&VRName_base::compileName, dynamic_cast<VRName_base*>(this))) );
}

VRName::~VRName() {}
