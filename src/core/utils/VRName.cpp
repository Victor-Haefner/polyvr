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

    regStorageSetupFkt( VRFunction<int>::create("name_update", boost::bind(&VRName_base::compileName, dynamic_cast<VRName_base*>(this))) );
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

void VRName_base::compileName() {
    setNameSpace(nameSpace);
    name = base_name;
    map<string, map<int, string> >& nameDict = nameDicts[nameSpace];
    if (name_suffix>0) name += separator + toString(name_suffix);
    if (nameDict.count(base_name) == 0) nameDict[base_name] = map<int, string>();
    nameDict[base_name][name_suffix] = name;
}

string VRName_base::setName(string name) {
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
    string s = name;
    std::replace( s.begin(), s.end(), ' ', '_');
    std::replace( s.begin(), s.end(), separator, ' ');
    stringstream ss(s);
    ss >> s;
    int j;
    if (ss >> j) {
        base_name = s;
        name_suffix = j;
        compileName();
        return this->name;
    }

    base_name = name; // set new base name

    // get suffix
    if (nameDict.count(base_name) == 0) name_suffix = 0;
    else {
        for(unsigned int i=0; i<=nameDict[base_name].size(); i++) {
            if (nameDict[base_name].count(i) == 1) continue;

            name_suffix = i;
            break;
        }
    }

    // finish
    compileName();
    return this->name;
}
string VRName_base::getName() { return name; }
string VRName_base::getBaseName() { return base_name; }
int VRName_base::getNameSuffix() { return name_suffix; }

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
    map<string, map<string, map<int, string> > >::iterator itr1;
    for (itr1 = nameDicts.begin(); itr1 != nameDicts.end(); itr1++) {
        N += itr1->second.size();
    }
    return N;
}

int VRName_base::getNameNumber() {
    int N = 0;
    map<string, map<string, map<int, string> > >::iterator itr1;
    map<string, map<int, string> >::iterator itr2;
    for (itr1 = nameDicts.begin(); itr1 != nameDicts.end(); itr1++) {
        for (itr2 = itr1->second.begin(); itr2 != itr1->second.end(); itr2++) {
            N += itr2->second.size();
        }
    }
    return N;
}

void VRName_base::printNameDict() { // call this to see what is not deleted -> add to hidden button?
    map<string, map<string, map<int, string> > >::iterator itr1;
    map<string, map<int, string> >::iterator itr2;
    map<int, string>::iterator itr3;
    for (itr1 = nameDicts.begin(); itr1 != nameDicts.end(); itr1++) {
        cout << "\n" << itr1->first << flush;
        for (itr2 = itr1->second.begin(); itr2 != itr1->second.end(); itr2++) {
            cout << "\n " << itr2->first << flush;
            for (itr3 = itr2->second.begin(); itr3 != itr2->second.end(); itr3++) {
                cout << "\n  " << itr3->second << flush;
            }
        }
    }
}
