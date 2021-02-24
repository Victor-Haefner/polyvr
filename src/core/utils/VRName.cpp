#include "VRName.h"
#include <vector>
#include <sstream>
#include <map>
#include <list>
#include <iostream>
#include "toString.h"
#include "VRStorage_template.h"
#include "VRFunction.h"

using namespace OSG;


struct VRNamePool {
    list<pair<int,int>> ranges; // ranges of suffixes

    VRNamePool() {
        ranges.push_back(make_pair(-1,-1));
    }

    void mergeTest(list<pair<int,int>>::iterator& itr1) {
        auto itr2 = itr1; itr2++;
        if (itr2 == ranges.end()) return;
        if (itr2->first == itr1->second) { // overlap
            itr1->second = itr2->second;
            ranges.erase(itr2);
            return;
        }
        if (itr2->first == itr1->second+1) { // continuity
            itr1->second = itr2->second;
            ranges.erase(itr2);
            return;
        }
    }

    int get() {
        auto itr1 = ranges.begin();
        int res = itr1->second+1;
        itr1->second = res;
        mergeTest(itr1);
        return res;
    }

    void add(int i) {
        for (auto itr1 = ranges.begin(); itr1 != ranges.end(); itr1++) {
            if (itr1->first <= i && itr1->second >= i) return; // already present
            if (itr1->second <= i-1) {
                auto itr2 = itr1; itr2++;
                if (itr1->second == i-1) { // extend range at end
                    itr1->second = i;
                    mergeTest(itr1);
                } else if (itr2->first == i+1) {  // extend range at beginning
                    itr2->first = i;
                    mergeTest(itr1);
                } else ranges.insert(++itr1, make_pair(i,i));
                return;
            }
        }
    }

    void sub(int i) {
        for (auto itr = ranges.begin(); itr != ranges.end(); itr++) {
            if (itr->first <= i && itr->second >= i) {
                if (itr->first == i) { itr->first++; return; }
                if (itr->second == i) { itr->second--; return; }
                auto np = make_pair(i+1, itr->second);
                itr->second = i-1;
                ranges.insert(++itr, np);
                return;
            }
        }
    }

    bool has(int i) {
        for (auto& range : ranges) {
            if (range.first <= i && range.second >= i) return true;
        }
        return false;
    }
};

VRNameSpace::VRNameSpace(string nspace) : nspace(nspace) {}

int VRNameSpace::getSuffix(const string& base, const int& hint) {
    if (!nameDict.count(base)) nameDict[base] = VRNamePool();
    auto& pool = nameDict[base];
    if (hint != -1) if (!pool.has(hint)) return hint;
    return pool.get();
}

void VRNameSpace::removeName(const string& base, const int& suffix) {
    if (nameDict.count(base)) nameDict[base].sub(suffix);
}

string VRNameSpace::compileName(const string& base, const int& suffix) {
    if (!unique) return base;
    string name = base;
    if (suffix > 0) name += separator + toString(suffix);
    if (!nameDict.count(base)) nameDict[base] = VRNamePool();
    nameDict[base].add(suffix);
    return name;
}

void VRNameSpace::setSeparator(char   s) { separator = s; }
void VRNameSpace::setUniqueNames(bool b) { unique = b; }
void VRNameSpace::filterNameChars(string chars, char replacement) { filter = chars; filter_rep = replacement; }

void VRNameSpace::applyFilter(string& name) {
    for (char c : filter) replace(name.begin(), name.end(),c,filter_rep);
}

int VRNameSpace::getNameNumber() { // TODO: currently the number of ranges!
    int N = 0;
    for (auto n : nameDict) N += n.second.ranges.size();
    return N;
}

void VRNameSpace::print() {
    for (auto n : nameDict) {
        cout << "\n " << n.first << flush;
        for (auto r : n.second.ranges) cout << "\n  " << r.first << " " << r.second << flush;
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

VRNameSpace* VRName::setNameSpace(string s) {
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

VRNameSpace* VRName::resetNameSpace() { return setNameSpace("__global__"); }

void VRName::compileName() {
    if (!nameSpace) setNameSpace(nameSpaceName);
    name = nameSpace->compileName(base_name, name_suffix);
}

string VRName::setName(string name) {
    if (!nameSpace) setNameSpace(nameSpaceName);
    nameSpace->applyFilter(name);
    if (base_name == name && name_suffix == 0) return this->name; // already named like that, return
    nameSpace->removeName(base_name, name_suffix); // check if already named, remove old name from dict
    if (name == "") return "";

    if (nameSpace->unique) {
        // check if passed name has a base|separator|suffix structure
        auto vs = splitString(name, nameSpace->separator);
        if (vs.size() > 1) {
            string last = *vs.rbegin();
            bool isNumber = (last.find_first_not_of( "0123456789" ) == string::npos);
            if (isNumber) {
                base_name = vs[0];
                for (unsigned int i=1; i<vs.size()-1; i++) base_name += nameSpace->separator + vs[i];
                name_suffix = nameSpace->getSuffix(base_name, toInt(last));
                compileName();
                return this->name;
            }
        }
        base_name = name;
        name_suffix = nameSpace->getSuffix(base_name);
    } else {
        base_name = name;
    }

    compileName();
    return this->name;
}

string VRName::getName() { return name; }
string VRName::getBaseName() { return base_name; }
int VRName::getNameSuffix() { return name_suffix; }

VRName::VRName() {
    store("name_suffix", &name_suffix);
    store("base_name", &base_name);
    store("name_space", &nameSpaceName);

    regStorageSetupFkt( VRStorageCb::create("name_update", bind(&VRName::setup, dynamic_cast<VRName*>(this), placeholders::_1)) );
}

VRName::~VRName() { setName(""); }

void VRName::setup(VRStorageContextPtr context) { compileName(); }

