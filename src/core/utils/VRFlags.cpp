#include "VRFlags.h"
#include <iostream>

VRFlags::VRFlags() {}

void VRFlags::addFlag(int f) { if ( !hasFlag(f) ) setFlag(f, false); }
bool VRFlags::hasFlag(int f) { return (flags.find(f) != flags.end() ); }
bool VRFlags::getFlag(int f) { if ( hasFlag(f) ) return flags[f]; return false; }
void VRFlags::setFlag(int f, bool b) { flags[f] = b; }

void VRFlags::printFlags() {
    for (auto f : flags) std::cout << " Flag " << f.first << " set to " << f.second << std::endl;
}
