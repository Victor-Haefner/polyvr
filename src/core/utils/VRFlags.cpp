#include "VRFlags.h"
#include <iostream>

using namespace std;

// global flags library
int flag_counter = 0;
unordered_map<string, int> flag_IDs;
unordered_map<int, string> flag_names;

bool flagExists(string name) { return (flag_IDs.find(name) != flag_IDs.end() ); }
bool flagExists(int ID) { return (flag_names.find(ID) != flag_names.end() ); }
string flagName(int ID) { return flagExists(ID) ? flag_names[ID] : ""; }
int flagID(string name) { return flagExists(name) ? flag_IDs[name] : 0; }
void flagAdd(string name) {
    if (flagExists(name)) return;
    flag_counter++;
    flag_IDs[name] = flag_counter;
    flag_names[flag_counter] = name;
}

// flags manager
VRFlags::VRFlags() {}

bool VRFlags::hasFlag(string flag) { return (flags.find( flagID(flag) ) != flags.end() ); }

bool VRFlags::getFlag(string flag) {
    int f = flagID(flag);
    return hasFlag(flag) ? flags[f] : false;
}

void VRFlags::setFlag(string flag, bool b) {
    flagAdd(flag);
    flags[ flagID(flag) ] = b;
}

void VRFlags::printFlags() {
    for (auto f : flags) std::cout << " Flag " << flagName(f.first) << " set to " << f.second << std::endl;
}
