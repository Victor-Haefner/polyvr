#include "VRReasoner.h"
#include "VROntology.h"

#include <iostream>

using namespace std;

VRReasoner::VRReasoner() {;}

VRReasoner* VRReasoner::get() {
    static VRReasoner* r = new VRReasoner();
    return r;
}

string VRReasoner::Result::toString() {
    //if (o) return o->toString();
    return "";
}


VRReasoner::Result VRReasoner::process(string query) {
    cout << "VRReasoner query: " << query << endl;
    Result res;
    return res;
}
