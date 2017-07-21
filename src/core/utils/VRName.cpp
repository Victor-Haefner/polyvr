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

// namespace   basename   suffix  name
map<string, map<string, map<int, string> > > nameDicts;

VRName::VRName() {
    store("name_suffix", &name_suffix);
    store("base_name", &base_name);
    store("name_space", &nameSpace);
    store("unique", &unique);

    regStorageSetupFkt( VRUpdateCb::create("name_update", boost::bind(&VRName_base::compileName, dynamic_cast<VRName_base*>(this))) );
}

VRName::~VRName() {}

VRName_base::VRName_base() {
    if (nameDicts.count(nameSpace) == 0) nameDicts[nameSpace] = map<string, map<int, string> >();
}

VRName_base::~VRName_base() { setName(""); }

void VRName_base::setSeparator(char   s) { separator = s; }
void VRName_base::setNameSpace(string s) {
    if (nameSpace == s) return;

    map<string, map<int, string> >& nameDict = nameDicts[nameSpace];
    if (nameDict.count(base_name)) nameDict[base_name].erase(name_suffix);

    nameSpace = s;
    if (nameDicts.count(nameSpace) == 0) nameDicts[nameSpace] = map<string, map<int, string> >();
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
    map<string, map<int, string> >& nameDict = nameDicts[nameSpace];
    if (name_suffix>0 && unique) name += separator + toString(name_suffix);
    if (nameDict.count(base_name) == 0 && unique) nameDict[base_name] = map<int, string>();
    if (unique) nameDict[base_name][name_suffix] = name;
}

string VRName_base::setName(string name) {
    for (char c : filter) replace(name.begin(), name.end(),c,filter_rep);
    //if (name == "arg") cout << "\n SET NAME " << name << " " << this->name << " " << nameSpace << flush;

    if (base_name == name && name_suffix == 0) {
        return this->name; // allready named like that, return
    }

    map<string, map<int, string> >& nameDict = nameDicts[nameSpace];

    // remove name from when passed name is empty
    if (name == "" && nameDict.count(base_name) == 1) {
        nameDict[base_name].erase(name_suffix);
        return "";
    }

    if (nameDict.count(base_name) == 1) nameDict[base_name].erase(name_suffix); // check if allready named, remove old name from dict

    // check if passed name has a base . suffix structure
    auto vs = splitString(name, separator);
    if (vs.size() > 1) {
        string last = *vs.rbegin();
        bool isNumber = (last.find_first_not_of( "0123456789" ) == string::npos);
        if (isNumber) {
            base_name = vs[0];
            for (uint i=1; i<vs.size()-1; i++) base_name += separator + vs[i];
            name_suffix = toInt(last);
            compileName();
            return this->name;
        }
    }

    base_name = name; // set new base name

    // get suffix
    if (unique) {
        if (nameDict.count(base_name) == 0) name_suffix = 0;
        else {
            for(unsigned int i=0; i<=nameDict[base_name].size(); i++) {
                if (nameDict[base_name].count(i) == 1) continue;
                name_suffix = i;
                break;
            }
        }
    }

    // finish
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
    map<string, map<int, string> >& nameDict = nameDicts[nameSpace];
    if (nameDict.count(base_name) == 1) nameDict[base_name].erase(name_suffix); // check if allready named, remove old name from dict

    if (e->get_attribute("name_suffix")) name_suffix = toInt(e->get_attribute("name_suffix")->get_value());
    if (e->get_attribute("base_name")) base_name = e->get_attribute("base_name")->get_value();
    if (e->get_attribute("name_space")) nameSpace = e->get_attribute("name_space")->get_value();
    compileName();
}



int VRName_base::getBaseNameNumber() {
    int N = 0;
    for (auto n : nameDicts) N += n.second.size();
    return N;
}

int VRName_base::getNameNumber() {
    int N = 0;
    for (auto n : nameDicts) {
        for (auto n2 : n.second) N += n2.second.size();
    }
    return N;
}

void VRName_base::printNameDict() { // call this to see what is not deleted -> add to hidden button?
    for (auto n : nameDicts) {
        cout << "\n" << n.first << flush;
        for (auto n2 : n.second) {
            cout << "\n " << n2.first << flush;
            for (auto n3 : n2.second) cout << "\n  " << n3.second << flush;
        }
    }
}
