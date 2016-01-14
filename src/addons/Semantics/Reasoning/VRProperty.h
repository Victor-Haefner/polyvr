#ifndef VRPROPERTY_H_INCLUDED
#define VRPROPERTY_H_INCLUDED

#include "VROntologyUtils.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;

struct VRProperty : public VRNamedID {
    string type;
    string value;

    VRProperty(string name, string type = "");
    static VRPropertyPtr create(string name, string type = "");
    VRPropertyPtr copy();

    void setType(string type);

    string toString();
};

#endif
